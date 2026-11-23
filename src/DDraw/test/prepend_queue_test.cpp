#include "../PrependQueue.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

namespace
{
    struct Engine
    {
        struct Request
        {
            int action;
            int target;
            int x, y, z;
            int p1, p2, p3;
            int rotation;
            Request(int value = 0) : action(value), target(0), x(0), y(0), z(0),
                p1(0), p2(0), p3(0), rotation(0) {}
        };
        struct Node
        {
            Request request;
            Node* next;
            bool boundary;
            int runningState;
            explicit Node(const Request& value) : request(value), next(NULL),
                boundary(false), runningState(0) {}
        };
        struct Unit { Node* head; Unit() : head(NULL) {} };

        int allocations, failAllocation, constructs, failConstruct, removals;
        int rawLive, constructedLive, destroyedUnlinked, snapshots;
        PrependQueue::CurrentKind currentKind;

        Engine() : allocations(0), failAllocation(0), constructs(0), failConstruct(0),
            removals(0), rawLive(0), constructedLive(0), destroyedUnlinked(0),
            snapshots(0), currentKind(PrependQueue::CurrentKind::Resume) {}

        Node* Head(Unit& unit) { return unit.head; }
        Node* Next(Node* node) { assert(node); return node->next; }
        bool Boundary(Node* node) { return node->boundary; }
        void SetBoundary(Node* node, bool value) { node->boundary = value; }
        PrependQueue::CurrentKind SnapshotResume(Unit&, Node* node, Request& request)
        {
            ++snapshots;
            request = node->request;
            return currentKind;
        }
        void* AllocateRaw()
        {
            ++allocations;
            if (allocations == failAllocation)
                return NULL;
            void* raw = std::malloc(sizeof(Node));
            assert(raw);
            ++rawLive;
            return raw;
        }
        void FreeRaw(void* raw) { --rawLive; std::free(raw); }
        Node* Construct(Unit&, void* raw, const Request& request)
        {
            ++constructs;
            if (constructs == failConstruct)
                return NULL;
            --rawLive;
            ++constructedLive;
            return new(raw) Node(request);
        }
        void DestroyUnlinked(Unit&, Node* node)
        {
            ++destroyedUnlinked;
            --constructedLive;
            node->~Node();
            std::free(node);
        }
        void Remove(Unit& unit, Node* node)
        {
            ++removals;
            Node** link = &unit.head;
            while (*link && *link != node)
                link = &(*link)->next;
            assert(*link == node);
            *link = node->next;
            DestroyUnlinked(unit, node);
        }
        void InsertBefore(Unit& unit, Node* node, Node* before)
        {
            Node** link = &unit.head;
            while (*link && *link != before)
                link = &(*link)->next;
            assert(*link == before);
            node->next = before;
            *link = node;
        }
        Node* Add(Unit& unit, int action, bool boundary = false)
        {
            Node* node = new(std::malloc(sizeof(Node))) Node(Request(action));
            ++constructedLive;
            node->boundary = boundary;
            InsertBefore(unit, node, NULL);
            return node;
        }
        void Clear(Unit& unit)
        {
            while (unit.head)
            {
                Node* node = unit.head;
                unit.head = node->next;
                DestroyUnlinked(unit, node);
            }
            assert(rawLive == 0 && constructedLive == 0);
        }
        // Model the native append boundary: insert immediately after the marker,
        // then transfer that marker onto the appended command.
        void Append(Unit& unit, int action)
        {
            Node* boundary = unit.head;
            while (boundary && !boundary->boundary)
                boundary = boundary->next;
            assert(boundary);
            Node* before = boundary->next;
            boundary->boundary = false;
            Node* node = new(std::malloc(sizeof(Node))) Node(Request(action));
            ++constructedLive;
            node->boundary = true;
            InsertBefore(unit, node, before);
        }
    };

    typedef PrependQueue::Cursor<Engine> Cursor;
    typedef PrependQueue::Result Result;

    void Expect(Engine::Unit& unit, const std::vector<int>& actions)
    {
        Engine::Node* node = unit.head;
        for (std::size_t i = 0; i < actions.size(); ++i)
        {
            assert(node && node->request.action == actions[i]);
            node = node->next;
        }
        assert(!node);
    }

    void ResumeAndBatch()
    {
        Engine engine;
        Engine::Unit unit;
        Engine::Node* old = engine.Add(unit, 10);
        old->runningState = 99;
        old->request.target = 42;
        old->request.x = 123;
        old->request.y = -7;
        old->request.z = 456;
        old->request.p1 = 11;
        old->request.p2 = 12;
        old->request.p3 = 13;
        old->request.rotation = 3;
        Engine::Node* pending = engine.Add(unit, 20, true);
        Cursor cursor;
        assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::Inserted);
        Expect(unit, std::vector<int>{1, 10, 20});
        assert(cursor.anchor->next == pending && cursor.anchor->runningState == 0);
        const Engine::Request& resumed = cursor.anchor->request;
        assert(resumed.target == 42 && resumed.x == 123 && resumed.y == -7 && resumed.z == 456);
        assert(resumed.p1 == 11 && resumed.p2 == 12 && resumed.p3 == 13 && resumed.rotation == 3);
        assert(!unit.head->boundary && !cursor.anchor->boundary && pending->boundary);
        assert(engine.removals == 1 && engine.snapshots == 1 && engine.allocations == 2);
        // A reclaim/build pair or grid is one ordered block, not a stack.
        assert(PrependQueue::Insert(engine, unit, Engine::Request(2), cursor) == Result::Inserted);
        assert(PrependQueue::Insert(engine, unit, Engine::Request(3), cursor) == Result::Inserted);
        assert(PrependQueue::Insert(engine, unit, Engine::Request(4), cursor) == Result::Inserted);
        Expect(unit, std::vector<int>{1, 2, 3, 4, 10, 20});
        assert(engine.removals == 1 && engine.snapshots == 1);
        engine.Append(unit, 30);
        Expect(unit, std::vector<int>{1, 2, 3, 4, 10, 20, 30});
        engine.Clear(unit);
    }

    void BoundaryAndIdle()
    {
        {
            Engine engine;
            Engine::Unit unit;
            engine.Add(unit, 10, true);
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::Inserted);
            assert(!unit.head->boundary && cursor.anchor->boundary);
            engine.Append(unit, 20);
            Expect(unit, std::vector<int>{1, 10, 20});
            engine.Clear(unit);
        }
        {
            Engine engine;
            engine.currentKind = PrependQueue::CurrentKind::Idle;
            Engine::Unit unit;
            Engine::Node* idle = engine.Add(unit, 0, true);
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::Inserted);
            assert(cursor.anchor == idle && engine.removals == 0 && engine.allocations == 1);
            assert(!unit.head->boundary && idle->boundary);
            engine.Append(unit, 20);
            Expect(unit, std::vector<int>{1, 0, 20});
            engine.Clear(unit);
        }
        {
            Engine engine;
            Engine::Unit unit;
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::Inserted);
            Engine::Node* first = unit.head;
            assert(!cursor.anchor && first->boundary && engine.snapshots == 0);
            assert(PrependQueue::Insert(engine, unit, Engine::Request(2), cursor) == Result::Inserted);
            assert(!first->boundary && first->next->boundary);
            engine.Append(unit, 3);
            Expect(unit, std::vector<int>{1, 2, 3});
            engine.Clear(unit);
        }
    }

    void FailedPreflight()
    {
        for (int fail = 1; fail <= 2; ++fail)
        {
            Engine engine;
            Engine::Unit unit;
            Engine::Node* old = engine.Add(unit, 10, true);
            old->runningState = 77;
            Cursor cursor;
            engine.failAllocation = fail;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::AllocationFailed);
            assert(unit.head == old && old->boundary && old->runningState == 77);
            assert(!cursor.initialized && engine.constructs == 0 && engine.removals == 0 && engine.rawLive == 0);
            engine.Clear(unit);
        }
        for (int fail = 1; fail <= 2; ++fail)
        {
            Engine engine;
            Engine::Unit unit;
            Engine::Node* old = engine.Add(unit, 10, true);
            Cursor cursor;
            engine.failConstruct = fail;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::AllocationFailed);
            assert(unit.head == old && old->boundary && !cursor.initialized && engine.removals == 0);
            assert(engine.rawLive == 0 && engine.constructedLive == 1);
            engine.Clear(unit);
        }
        {
            Engine engine;
            engine.currentKind = PrependQueue::CurrentKind::Unsupported;
            Engine::Unit unit;
            Engine::Node* old = engine.Add(unit, 99, true);
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::Unsupported);
            assert(unit.head == old && engine.allocations == 0 && engine.removals == 0 && !cursor.initialized);
            engine.Clear(unit);
        }
        {
            Engine engine;
            Engine::Unit unit;
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::Inserted);
            Engine::Node* first = unit.head;
            engine.failAllocation = engine.allocations + 1;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(2), cursor) == Result::AllocationFailed);
            assert(cursor.lastPrefix == first && first->boundary && !first->next);
            engine.Clear(unit);
        }
    }

    void InvalidQueues()
    {
        {
            Engine engine;
            Engine::Unit unit;
            Engine::Node* first = engine.Add(unit, 10);
            Engine::Node* last = engine.Add(unit, 20, true);
            last->next = first;
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::InvalidQueue);
            assert(engine.allocations == 0 && engine.snapshots == 0);
            last->next = NULL;
            engine.Clear(unit);
        }
        {
            Engine engine;
            Engine::Unit unit;
            Engine::Node* first = engine.Add(unit, 10, true);
            first->next = first;
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::InvalidQueue);
            first->next = NULL;
            engine.Clear(unit);
        }
        {
            Engine engine;
            Engine::Unit unit;
            // Build directly so the test's setup is linear too.
            for (std::size_t i = 0; i < PrependQueue::MaxOrders + 1; ++i)
            {
                Engine::Node* node = new(std::malloc(sizeof(Engine::Node))) Engine::Node(Engine::Request(10));
                ++engine.constructedLive;
                node->next = unit.head;
                unit.head = node;
            }
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::InvalidQueue);
            engine.Remove(unit, unit.head); // Exactly full queues cannot grow either.
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::InvalidQueue);
            assert(engine.allocations == 0 && !cursor.initialized);
            engine.Clear(unit);
        }
        {
            Engine engine;
            Engine::Unit unit;
            engine.Add(unit, 10, true);
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::Inserted);
            engine.Remove(unit, cursor.anchor);
            // The dangling anchor is compared only; it is never dereferenced.
            const int allocations = engine.allocations;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(2), cursor) == Result::InvalidQueue);
            assert(engine.allocations == allocations);
            engine.Clear(unit);
        }
        {
            Engine engine;
            Engine::Unit unit, other;
            Cursor cursor;
            assert(PrependQueue::Insert(engine, unit, Engine::Request(1), cursor) == Result::Inserted);
            assert(PrependQueue::Insert(engine, other, Engine::Request(2), cursor) == Result::InvalidQueue);
            engine.Remove(unit, unit.head);
            assert(PrependQueue::Insert(engine, unit, Engine::Request(2), cursor) == Result::InvalidQueue);
            engine.Clear(unit);
        }
    }
}

int main()
{
    ResumeAndBatch();
    BoundaryAndIdle();
    FailedPreflight();
    InvalidQueues();
    std::cout << "prepend queue policy tests passed\n";
}
