#pragma once

#include "DialogBase.h"
#include "VoteReject.h"
#include "Widgets/Button.h"
#include "Widgets/Label.h"

#include <memory>
#include <string>
#include <vector>
#include <windows.h>


class VoteDialog : public DialogBase
{
public:
    VoteDialog(bool vidMem);
    ~VoteDialog();

    // Rebuild rows from VoteReject's current active votes.
    // Shows the dialog if any votes are active; hides it if there are none.
    // Called by VoteReject whenever the vote set changes structurally.
    void Refresh();

    void Hide();
    bool IsVisible() const { return m_visible; }

    // BlitDialog and Message are inherited from DialogBase.

private:
    void RenderDialog() override;
    void RestoreAll() override;

    struct VoteRow {
        unsigned                targetDpid;
        std::shared_ptr<Label>  titleLabel;
        std::shared_ptr<Label>  statusLabel;
        std::shared_ptr<Button> yesButton;
        std::shared_ptr<Button> noButton;
        std::shared_ptr<Button> takeButton;  // present only for ally timeout votes
    };

    // Rebuild m_widgets and m_voteRows from the given vote snapshots.
    // Also updates m_dialogHeight to cover exactly the active rows.
    void RebuildRows(const std::vector<VoteReject::VoteDisplayInfo>& votes);

    // Button skin surface (fonts/background/cursor are in DialogBase).
    LPDIRECTDRAWSURFACE m_lpButtonSkin;

    std::vector<VoteRow> m_voteRows;
};

extern VoteDialog* g_VoteDialog;
