#include "PresetBar.h"

PresetBar::PresetBar (PresetManager& m) : manager (m)
{
    auto setup = [&] (juce::TextButton& btn)
    {
        btn.setColour (juce::TextButton::buttonColourId,    juce::Colour (0xff2a2a32));
        btn.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff3a3a48));
        btn.setColour (juce::TextButton::textColourOffId,   juce::Colours::white.withAlpha (0.8f));
        btn.setColour (juce::ComboBox::outlineColourId,     juce::Colours::white.withAlpha (0.1f));
        addAndMakeVisible (btn);
    };

    setup (prevButton);
    setup (nextButton);
    setup (nameButton);
    setup (initButton);
    setup (saveButton);

    refresh();

    prevButton.onClick = [this]
    {
        manager.navigate (-1);
        refresh();
        if (auto* editor = getParentComponent())
            editor->repaint();
    };

    nextButton.onClick = [this]
    {
        manager.navigate (+1);
        refresh();
        if (auto* editor = getParentComponent())
            editor->repaint();
    };

    nameButton.onClick = [this] { showPresetMenu(); };

    initButton.onClick = [this]
    {
        manager.loadFactoryPreset (0);   // index 0 is always "Init"
        refresh();
    };

    saveButton.onClick = [this] { showSaveDialog(); };
}

// ──────────────────────────────────────────────────────────────────────────

void PresetBar::refresh()
{
    nameButton.setButtonText (manager.getCurrentPresetName());
}

void PresetBar::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff18181e));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 4.0f, 1.0f);
}

void PresetBar::resized()
{
    auto area = getLocalBounds().reduced (4, 3);

    prevButton.setBounds (area.removeFromLeft (28));
    area.removeFromLeft (2);

    nextButton.setBounds (area.removeFromLeft (28));
    area.removeFromLeft (4);

    saveButton.setBounds (area.removeFromRight (60));
    area.removeFromRight (4);
    initButton.setBounds (area.removeFromRight (48));
    area.removeFromRight (6);

    nameButton.setBounds (area); // name button fills the remaining centre
}

// ──────────────────────────────────────────────────────────────────────────

void PresetBar::showPresetMenu()
{
    juce::PopupMenu menu;

    // ── Factory presets ────────────────────────────────────────────────────
    juce::PopupMenu factoryMenu;
    for (int i = 0; i < manager.getNumFactoryPresets(); ++i)
    {
        auto name = manager.getFactoryPresetName (i);
        bool isCurrent = (name == manager.getCurrentPresetName());
        factoryMenu.addItem (i + 1, name, true, isCurrent);
    }
    menu.addSubMenu ("Factory", factoryMenu);

    // ── User presets ───────────────────────────────────────────────────────
    auto userNames = manager.getUserPresetNames();
    if (userNames.isEmpty())
    {
        menu.addItem (-1, "(no saved presets)", false);
    }
    else
    {
        juce::PopupMenu userMenu;
        int startId = 1000;
        for (int i = 0; i < userNames.size(); ++i)
        {
            bool isCurrent = (userNames[i] == manager.getCurrentPresetName());
            userMenu.addItem (startId + i, userNames[i], true, isCurrent);
        }
        menu.addSubMenu ("My Presets", userMenu);

        // Delete submenu
        juce::PopupMenu deleteMenu;
        for (int i = 0; i < userNames.size(); ++i)
            deleteMenu.addItem (2000 + i, userNames[i]);
        menu.addSeparator();
        menu.addSubMenu ("Delete preset\u2026", deleteMenu);
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (nameButton),
        [this, userNames] (int result)
        {
            if (result <= 0) return;

            if (result < 1000)
            {
                // Factory preset
                manager.loadFactoryPreset (result - 1);
                refresh();
            }
            else if (result < 2000)
            {
                // User preset
                auto name = userNames[result - 1000];
                manager.loadUserPreset (name);
                refresh();
            }
            else
            {
                // Delete
                auto name = userNames[result - 2000];
                juce::AlertWindow::showOkCancelBox (
                    juce::MessageBoxIconType::WarningIcon,
                    "Delete Preset",
                    "Delete \"" + name + "\"?",
                    "Delete", "Cancel", nullptr,
                    juce::ModalCallbackFunction::create ([this, name] (int res)
                    {
                        if (res == 1)
                        {
                            manager.deleteUserPreset (name);
                            refresh();
                        }
                    }));
            }
        });
}

void PresetBar::showSaveDialog()
{
    auto* window = new juce::AlertWindow ("Save Preset",
                                           "Enter a name for this preset:",
                                           juce::MessageBoxIconType::NoIcon);
    window->addTextEditor   ("name", manager.getCurrentPresetName());
    window->addButton       ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton       ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true,
        juce::ModalCallbackFunction::create ([this, window] (int result)
        {
            if (result == 1)
            {
                auto name = window->getTextEditorContents ("name").trim();
                if (name.isNotEmpty())
                {
                    manager.saveUserPreset (name);
                    refresh();
                }
            }
            delete window;
        }),
        true /* deleteWhenDismissed = false because we delete manually */);
}
