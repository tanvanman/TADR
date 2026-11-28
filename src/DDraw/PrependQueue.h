#ifndef TADR_PREPEND_QUEUE_H
#define TADR_PREPEND_QUEUE_H

#include <cstddef>

// Queue policy only: no native addresses, allocation implementation or input state.
// Execute synchronously on the engine thread. Adapter calls must not throw or
// re-enter queue mutation. The caller must also check the unit's live identity
// before reusing a Cursor; a Unit* alone cannot detect a recycled native unit.
//
// Engine supplies Node, Unit and a default-constructible Request, plus:
//   Node* Head(Unit&), Node* Next(Node*)
//   bool Boundary(Node*), void SetBoundary(Node*, bool)
//   CurrentKind SnapshotResume(Unit&, Node* current, Request& freshResume)
//   void* AllocateRaw(), void FreeRaw(void*)
//   Node* Construct(Unit&, void* raw, const Request&)
//   void DestroyUnlinked(Unit&, Node*)
//   void Remove(Unit&, Node*), void InsertBefore(Unit&, Node*, Node* before)
//
// SnapshotResume must reject handlers that cannot safely be restarted. Resume
// requests describe fresh work, including any native build/rotation conversion;
// they must never copy a running handler's state or embedded target references.
// Idle means a harmless existing idle node that can remain behind urgent work.
// Construct initializes an unlinked node, including its owner and target refs,
// without changing the unit queue. It normally cannot fail after AllocateRaw;
// if it returns null, raw must remain unconstructed and owned by the caller.
// DestroyUnlinked releases native references and frees the constructed node.
// Remove removes/destroys ONLY that node and preserves its pending tail.
// InsertBefore links an already constructed node; a null before appends.
// Remove and InsertBefore must be infallible: no allocation occurs after the
// first queue mutation. The native adapter owns handler cleanup semantics.
namespace PrependQueue
{
    enum class Result { Inserted, Unsupported, AllocationFailed, InvalidQueue };
    enum class CurrentKind { Resume, Idle, Unsupported };

    static const std::size_t MaxOrders = 8192;

    template<class Engine>
    struct Cursor
    {
        typedef typename Engine::Unit Unit;
        typedef typename Engine::Node Node;

        Unit* unit;
        Node* anchor;
        Node* lastPrefix;
        bool initialized;

        Cursor() : unit(NULL), anchor(NULL), lastPrefix(NULL), initialized(false) {}
    };

    namespace Detail
    {
        template<class Engine>
        struct QueueInfo
        {
            std::size_t count;
            bool hasAnchor;
            bool hasLastPrefix;

            QueueInfo() : count(0), hasAnchor(false), hasLastPrefix(false) {}
        };

        // Brent's cycle check needs constant memory and bounds every traversal,
        // including a corrupt list that never reaches null.
        template<class Engine>
        bool Inspect(Engine& engine, typename Engine::Node* head,
                     typename Engine::Node* anchor,
                     typename Engine::Node* lastPrefix, QueueInfo<Engine>& info)
        {
            typedef typename Engine::Node Node;
            Node* checkpoint = head;
            std::size_t power = 1;
            std::size_t distance = 0;
            for (Node* node = head; node; )
            {
                if (info.count == MaxOrders)
                    return false;
                ++info.count;
                info.hasAnchor = info.hasAnchor || node == anchor;
                info.hasLastPrefix = info.hasLastPrefix || node == lastPrefix;
                Node* next = engine.Next(node);
                ++distance;
                if (next && next == checkpoint)
                    return false;
                if (distance == power)
                {
                    checkpoint = next;
                    power *= 2;
                    distance = 0;
                }
                node = next;
            }
            return true;
        }
    }

    template<class Engine>
    Result Insert(Engine& engine, typename Engine::Unit& unit,
                  const typename Engine::Request& request, Cursor<Engine>& cursor)
    {
        typedef typename Engine::Node Node;
        Node* head = engine.Head(unit);
        Detail::QueueInfo<Engine> info;
        if (!Detail::Inspect(engine, head, cursor.anchor, cursor.lastPrefix, info))
            return Result::InvalidQueue;

        // A batch owns a fixed anchor, not the currently executing head. This
        // preserves the order of multiple commands produced by one gesture.
        if (cursor.initialized)
        {
            if (cursor.unit != &unit || !info.hasLastPrefix ||
                (cursor.anchor && !info.hasAnchor) ||
                engine.Next(cursor.lastPrefix) != cursor.anchor ||
                info.count == MaxOrders)
                return Result::InvalidQueue;

            void* raw = engine.AllocateRaw();
            if (!raw)
                return Result::AllocationFailed;
            Node* urgent = engine.Construct(unit, raw, request);
            if (!urgent)
            {
                engine.FreeRaw(raw);
                return Result::AllocationFailed;
            }
            engine.SetBoundary(urgent, cursor.anchor == NULL);
            if (!cursor.anchor)
                engine.SetBoundary(cursor.lastPrefix, false);
            engine.InsertBefore(unit, urgent, cursor.anchor);
            cursor.lastPrefix = urgent;
            return Result::Inserted;
        }

        typename Engine::Request resumeRequest;
        CurrentKind kind = CurrentKind::Idle;
        if (head)
        {
            kind = engine.SnapshotResume(unit, head, resumeRequest);
            if (kind == CurrentKind::Unsupported)
                return Result::Unsupported;
        }
        // A resumed head replaces one node with two. Retaining an idle head or
        // starting from an empty list also adds exactly one node.
        if (info.count == MaxOrders)
            return Result::InvalidQueue;

        void* urgentRaw = engine.AllocateRaw();
        if (!urgentRaw)
            return Result::AllocationFailed;
        void* resumeRaw = NULL;
        if (head && kind == CurrentKind::Resume)
        {
            resumeRaw = engine.AllocateRaw();
            if (!resumeRaw)
            {
                engine.FreeRaw(urgentRaw);
                return Result::AllocationFailed;
            }
        }

        Node* urgent = engine.Construct(unit, urgentRaw, request);
        if (!urgent)
        {
            engine.FreeRaw(urgentRaw);
            if (resumeRaw)
                engine.FreeRaw(resumeRaw);
            return Result::AllocationFailed;
        }
        Node* resume = NULL;
        if (resumeRaw)
        {
            resume = engine.Construct(unit, resumeRaw, resumeRequest);
            if (!resume)
            {
                engine.FreeRaw(resumeRaw);
                engine.DestroyUnlinked(unit, urgent);
                return Result::AllocationFailed;
            }
        }

        Node* anchor = head;
        if (resume)
        {
            Node* pending = engine.Next(head);
            engine.SetBoundary(resume, engine.Boundary(head));
            engine.SetBoundary(urgent, false);
            engine.Remove(unit, head);
            engine.InsertBefore(unit, resume, pending);
            anchor = resume;
        }
        else
        {
            // Empty queues need a boundary on the last urgent command. For an
            // existing idle head, retain its boundary and any pending work.
            engine.SetBoundary(urgent, head == NULL);
        }
        engine.InsertBefore(unit, urgent, anchor);
        cursor.unit = &unit;
        cursor.anchor = anchor;
        cursor.lastPrefix = urgent;
        cursor.initialized = true;
        return Result::Inserted;
    }
}

#endif
