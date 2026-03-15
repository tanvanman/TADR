#include "VoteDialog.h"
#include "VoteReject.h"
#include "iddrawsurface.h"
#include "pcxread.h"
#include "tamem.h"
#include "tafunctions.h"
#include "Widgets/Button.h"
#include "Widgets/Label.h"

#include <cstdio>

// -----------------------------------------------------------------------
// Layout constants
// -----------------------------------------------------------------------
// VOTE_DLG_W = 530,  max height = VOTE_DLG_H (9 votes + padding)
// Per vote (rowBase = voteIndex * 54):
//   Title  label:  x=10,  y=TOP_PAD+rowBase+2,  510px wide
//   Status label:  x=10,  y=TOP_PAD+rowBase+20, 190px wide
//   YES button:    x=208, y=TOP_PAD+rowBase+18  (96px wide skin)
//   NO  button:    x=314, y=TOP_PAD+rowBase+18  (96px wide skin, 10px gap)
//   .take button:  x=420, y=TOP_PAD+rowBase+18  (96px wide skin, 10px gap, 10px right margin)
//                  only present for ally timeout votes (rejectMask==6 && isAllyOfLocal)
//   Padding row:   y=TOP_PAD+rowBase+36..rowBase+53 (visual separation between votes)
// -----------------------------------------------------------------------

static const int VOTE_DLG_W         = 530;
static const int VOTE_DLG_MAX_VOTES = 9;
static const int VOTE_DLG_ROW_H     = 18;                               // one font row
static const int ROWS_PER_VOTE      = 3;                                // font rows per vote entry
static const int VOTE_ROW_H         = VOTE_DLG_ROW_H * ROWS_PER_VOTE;  // 54px per vote
static const int VOTE_DLG_TOP_PAD   = 8;                                // top margin inside dialog
static const int VOTE_DLG_BOT_PAD   = 8;                                // bottom margin inside dialog
static const int VOTE_DLG_H         = VOTE_DLG_MAX_VOTES * VOTE_ROW_H  // 502px max height
                                    + VOTE_DLG_TOP_PAD + VOTE_DLG_BOT_PAD;
// The PCX background has a 5-row grey border at top and bottom.  When it is
// stretched across VOTE_DLG_H pixels those 5 rows map to ~11px, so we copy
// the bottom 11 rows of the full-height stretched surface to m_dialogHeight-11
// so the border is visible even on a short (1-vote) dialog.
static const int VOTE_DLG_BORDER_H  = 11;
static const int BTN_YES_X          = 208;
static const int BTN_NO_X           = 314;
static const int BTN_TAKE_X         = 420;

VoteDialog* g_VoteDialog = nullptr;

VoteDialog::VoteDialog(bool vidMem)
    : DialogBase(vidMem, VOTE_DLG_W, VOTE_DLG_H, VOTE_DLG_ROW_H)
    , m_lpButtonSkin(nullptr)
{
    m_lpButtonSkin = CreateSurfPCXResource(12, vidMem);
    g_VoteDialog = this;
}

VoteDialog::~VoteDialog()
{
    if (m_lpButtonSkin) { m_lpButtonSkin->Release(); m_lpButtonSkin = nullptr; }
    if (g_VoteDialog == this) g_VoteDialog = nullptr;
}

// -----------------------------------------------------------------------
// Hide: make the dialog invisible.
// -----------------------------------------------------------------------
void VoteDialog::Hide()
{
    m_visible = false;
}

// -----------------------------------------------------------------------
// Refresh: resync rows with VoteReject's current active votes.
//   Shows the dialog if any votes are active; hides it if there are none.
// -----------------------------------------------------------------------
void VoteDialog::Refresh()
{
    std::vector<VoteReject::VoteDisplayInfo> votes;
    VoteReject::GetInstance()->GetActiveVotes(votes);

    // Check for structural change (vote added or removed).
    bool structChanged = (votes.size() != m_voteRows.size());
    if (!structChanged)
    {
        for (size_t i = 0; i < votes.size(); ++i)
        {
            if (votes[i].targetDpid != m_voteRows[i].targetDpid)
            {
                structChanged = true;
                break;
            }
        }
    }

    if (structChanged)
        RebuildRows(votes);

    if (m_voteRows.empty())
    {
        m_visible = false;
        return;
    }

    if (!m_visible)
    {
        RECT screenRect = (*TAmainStruct_PtrPtr)->GameSreen_Rect;
        posX = screenRect.left + (screenRect.right  - screenRect.left - VOTE_DLG_W) / 2;
        posY = screenRect.top  + (screenRect.bottom - screenRect.top) / 4;
        if (posX < screenRect.left) posX = screenRect.left;
        if (posY < screenRect.top)  posY = screenRect.top;
        m_visible = true;
    }
}

// -----------------------------------------------------------------------
// RebuildRows: clear and recreate VoteRow widgets from the given snapshots.
//   m_dialogHeight is updated to cover exactly the active rows.
// -----------------------------------------------------------------------
void VoteDialog::RebuildRows(const std::vector<VoteReject::VoteDisplayInfo>& votes)
{
    m_widgets.clear();
    m_voteRows.clear();

    int count = (int)votes.size();
    if (count > VOTE_DLG_MAX_VOTES)
        count = VOTE_DLG_MAX_VOTES;

    for (int i = 0; i < count; ++i)
    {
        const auto& v    = votes[i];
        int titleY       = i * VOTE_ROW_H + VOTE_DLG_TOP_PAD + 2;
        int statusY      = i * VOTE_ROW_H + VOTE_DLG_TOP_PAD + 20;
        int buttonY      = i * VOTE_ROW_H + VOTE_DLG_TOP_PAD + 18;
        unsigned dpid    = v.targetDpid;

        VoteRow row;
        row.targetDpid  = dpid;
        row.titleLabel  = std::make_shared<Label>(10, titleY,  510, 16, "");
        row.statusLabel = std::make_shared<Label>(10, statusY, 190, 14, "");

        row.yesButton = std::make_shared<Button>(
            BTN_YES_X, buttonY, m_lpButtonSkin, 0, 1, true,
            std::vector<std::string>{"YES"}, "",
            [dpid](int) {
                VoteReject::GetInstance()->CastLocalYesVote(dpid);
                if (g_VoteDialog) g_VoteDialog->Refresh();
            });

        row.noButton = std::make_shared<Button>(
            BTN_NO_X, buttonY, m_lpButtonSkin, 0, 1, true,
            std::vector<std::string>{"NO"}, "",
            [dpid](int) {
                VoteReject::GetInstance()->CastLocalNoVote(dpid);
                if (g_VoteDialog) g_VoteDialog->Refresh();
            });

        if (v.isAllyOfLocal && v.rejectMask == 6)
        {
            row.takeButton = std::make_shared<Button>(
                BTN_TAKE_X, buttonY, m_lpButtonSkin, 0, 1, true,
                std::vector<std::string>{".take"}, "",
                [](int) {
                    TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
                    ShowText(&taPtr->Players[taPtr->LocalHumanPlayer_PlayerID], ".take", 0, 0);
                });
        }

        m_widgets.push_back(row.titleLabel);
        m_widgets.push_back(row.statusLabel);
        m_widgets.push_back(row.yesButton);
        m_widgets.push_back(row.noButton);
        if (row.takeButton)
            m_widgets.push_back(row.takeButton);
        m_voteRows.push_back(std::move(row));
    }

    m_dialogHeight = count > 0
        ? count * VOTE_ROW_H + VOTE_DLG_TOP_PAD + VOTE_DLG_BOT_PAD
        : VOTE_DLG_H;
}

// -----------------------------------------------------------------------
// RestoreAll: restore shared surfaces + cursor (base), then button skin.
// -----------------------------------------------------------------------
void VoteDialog::RestoreAll()
{
    DialogBase::RestoreAll();

    if (m_lpButtonSkin)
    {
        m_lpButtonSkin->Restore();
        RestoreFromPCX(12, m_lpButtonSkin);
    }
}

// -----------------------------------------------------------------------
// RenderDialog: update label text and button state for each active row,
//   then render background + widgets via the base.
//   Also performs a structural check in case Refresh() was not called
//   after a vote state change.
// -----------------------------------------------------------------------
void VoteDialog::RenderDialog()
{
    std::vector<VoteReject::VoteDisplayInfo> votes;
    VoteReject::GetInstance()->GetActiveVotes(votes);

    // Safety structural check — normally Refresh() keeps rows in sync.
    bool structChanged = (votes.size() != m_voteRows.size());
    if (!structChanged)
    {
        for (size_t i = 0; i < votes.size(); ++i)
        {
            if (votes[i].targetDpid != m_voteRows[i].targetDpid)
            {
                structChanged = true;
                break;
            }
        }
    }
    if (structChanged)
        RebuildRows(votes);

    DWORD now = GetTickCount();

    for (auto& row : m_voteRows)
    {
        // Find this row's current vote info.
        const VoteReject::VoteDisplayInfo* info = nullptr;
        for (const auto& v : votes)
            if (v.targetDpid == row.targetDpid) { info = &v; break; }
        if (!info)
            continue;

        // Title
        if (info->rejectMask == 6)
            row.titleLabel->m_text = info->targetName + " has timed out - vote to reject?";
        else
            row.titleLabel->m_text = "VOTE: " + info->proposerName + " wants to reject " + info->targetName;

        // Status + button state
        int secsLeft = (now < info->expiryTime) ? (int)((info->expiryTime - now) / 1000) : 0;
        char buf[64];
        if (info->votingClosed)
        {
            _snprintf_s(buf, sizeof(buf), _TRUNCATE, "rejected - auto-rejects in %ds", secsLeft);
            row.yesButton->m_disabled = true;
            row.noButton->m_disabled  = true;
        }
        else
        {
            _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%dy/%dn/%d (%ds)",
                info->yesVotes, info->noVotes, info->votesNeeded, secsLeft);
            row.yesButton->m_disabled = false;
            row.noButton->m_disabled  = false;
        }
        row.statusLabel->m_text = buf;
    }

    DialogBase::RenderDialog();

    // Copy the bottom-border rows from the full-height stretched background
    // (which sits at the very bottom of lpDialogSurf) down to m_dialogHeight-BORDER_H
    // so the grey panel border is visible even on a short dialog.
    int srcY = VOTE_DLG_H - VOTE_DLG_BORDER_H;
    int dstY = m_dialogHeight - VOTE_DLG_BORDER_H;
    if (dstY >= 0 && dstY != srcY)
    {
        RECT src = { 0, srcY, m_dialogWidth, srcY + VOTE_DLG_BORDER_H };
        RECT dst = { 0, dstY, m_dialogWidth, dstY + VOTE_DLG_BORDER_H };
        if (lpDialogSurf->Blt(&dst, lpDialogSurf, &src, DDBLT_ASYNC, NULL) != DD_OK)
            lpDialogSurf->Blt(&dst, lpDialogSurf, &src, DDBLT_WAIT, NULL);
    }
}
