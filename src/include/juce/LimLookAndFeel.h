#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class LimLookAndFeel : public juce::LookAndFeel_V4 {
public:
    LimLookAndFeel(std::function<std::shared_ptr<Theme>()> theme)
        : theme_(std::move(theme))
    {

        // SelectionBackground - the light purple like when selecting items from the browser
        // ScrollbarInnerHandleHover - scrollbar thumb when not hovered
        // TreeColumnHeadControl - scrollbar thumb when hovered, probably. light light grey
        // SceneContrast -- the darker gray around the far borders and between elements
        // SurfaceBackground - the mid gray like the browser list background

        juce::File fontFile("/Applications/Ableton Live 12 Suite.app/Contents/App-Resources/Fonts/AbletonSansSmall-Regular.ttf");
        if (fontFile.existsAsFile()) {
            fontFile.loadFileAsData(fontData_);  // Load font data into MemoryBlock
            customTypeface_ = juce::Typeface::createSystemTypefaceFor(fontData_.getData(), fontData_.getSize());
            setDefaultSansSerifTypeface(customTypeface_);
        }

        // Set the default colors
        //
        // SurfaceHighlight -- lighter dark grey
        //
        //
        // SelectionBackground (Value="#b0ddeb")

    //    Hex: #b0ddeb (RGB: 176, 221, 235)
    //    This is a light blue color, slightly lighter than #A7C6D3, but close in tone.
    //
    //StandbySelectionBackground (Value="#637e86")
    //
    //    Hex: #637e86 (RGB: 99, 126, 134)
    //    This is a darker, more muted blue/gray, a bit darker than #A7C6D3.
    //
    //SelectorZoneBackground (Value="#bed6f4")
    //
    //    Hex: #bed6f4 (RGB: 190, 214, 244)
    //    This is a light blue, close to the desired color but a little lighter and more saturated.
        setColour(juce::TextEditor::backgroundColourId, theme_()->getColorValue("SurfaceBackground"));
        setColour(juce::TextEditor::textColourId, theme_()->getColorValue("ControlForeground"));

        setColour(juce::TextEditor::highlightColourId, theme_()->getColorValue("SelectionBackground"));
        setColour(juce::TextEditor::highlightedTextColourId, theme_()->getColorValue("ControlOnForeground"));
        setColour(juce::TextEditor::outlineColourId, theme_()->getColorValue("Desktop"));
        setColour(juce::TextEditor::focusedOutlineColourId, theme_()->getColorValue("ControlContrastFrame"));
        setColour(juce::TextEditor::shadowColourId, theme_()->getColorValue("ShadowDark"));

        setColour(juce::ListBox::backgroundColourId, theme_()->getColorValue("SurfaceBackground"));
        setColour(juce::ListBox::textColourId, theme_()->getColorValue("ControlForeground"));
        //setColour(juce::ListBox::outlineColourId, theme_()->getColorValue("ControlBackground"));

        setColour(juce::ScrollBar::backgroundColourId, theme_()->getColorValue("Desktop"));
        setColour(juce::ScrollBar::thumbColourId, theme_()->getColorValue("ScrollbarInnerHandleHover"));

    }

    void drawListBoxOutline(juce::Graphics& g, int width, int height);

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override {
        return customTypeface_;
    }

    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override {
        cursorColor_ = theme_()->getColorValue("Alert");
        auto caret = juce::LookAndFeel_V4::createCaretComponent(keyFocusOwner);
        if (caret != nullptr) {
            caret->setColour(juce::CaretComponent::caretColourId, cursorColor_);
        }
        return caret;
    }

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                       int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbStartPosition,
                       int thumbSize, bool isMouseOver, bool isMouseDown)
                       override
    {
        int customWidth = 18; // Adjust this value to your desired width

        // Calculate the center of the original scrollbar
        int centerX = x + width / 2;
        int centerY = y + height / 2;

        // Recalculate x position to center the custom width scrollbar
        int newX = centerX - customWidth / 2;

        // Draw scrollbar background
        //g.setColour(isMouseOver
        //            ? theme_()->getColorValue("SelectionBackground")
        //            : juce::Colours::transparentBlack
        //);
        //g.setColour(theme_()->getColorValue("Desktop"));
        //g.fillRect(newX, y, customWidth, height);
        //if (isScrollbarVertical)
        //{
        //    g.fillRect(newX, y, customWidth, height);
        //}
        //else
        //{
        //    g.fillRect(x, centerY - customWidth / 2, width, customWidth);
        //}

        // Draw scrollbar thumb
        g.setColour(theme_()->getColorValue("TreeColumnHeadControl"));

        float padding = 2.0f;
        //float cornerRadius = juce::jmin(width, height) * 0.5f;
        float cornerRadius = 2.0f;

        if (isScrollbarVertical) {
            thumbSize = (thumbSize * 0.9f) - (2 * padding);
            g.fillRoundedRectangle(newX + padding,
                                   thumbStartPosition + padding,
                                   customWidth - 2 * padding,
                                   thumbSize,
                                   cornerRadius);
        } else {
            g.fillRoundedRectangle(thumbStartPosition + padding,
                                   y + padding,
                                   thumbSize - 2 * padding,
                                   height - 2 * padding,
                                   cornerRadius);
            }
    }


//    void drawListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected,
//                         const juce::String& text, juce::ListBox& listBox);
//
//    void drawListBox(juce::Graphics& g, juce::ListBox& listBox,
//                     juce::BorderSize<int> border, int outlineThickness);
//
//    void drawListBoxBackground(juce::Graphics& g, juce::ListBox& listBox);

    void drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override {
        // Set the color for the outline (use different colors if it's focused or not)
        juce::Colour outlineColour = textEditor.hasKeyboardFocus (true)
                                     ? theme_()->getColorValue("ControlForeground")
                                     : theme_()->getColorValue("SelectionForeground");

        // Set the thickness of the border
        const float borderThickness = 1.0f;  // Customize the border thickness

        // Draw the rectangle outline
        g.setColour (outlineColour);
        g.drawRect(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), borderThickness);
    }


private:
    std::function<std::shared_ptr<Theme>()> theme_;
    juce::MemoryBlock fontData_;
    juce::Typeface::Ptr customTypeface_;
    juce::Colour cursorColor_;
};


// Implementation


//void drawListBoxOutline(juce::Graphics& g, int width, int height) {
//    // Set the outline color and draw the outline
//    g.setColour(theme_()->getColorValue("ControlContrastFrame"));
//    g.drawRect(0, 0, width, height, 2); // Draw a 2px border around the ListBox
//}

    // Override other methods if you want to change default styles
//    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override {
//        return juce::Font("Arial", 16.0f, juce::Font::bold);
//    }

    // Example: Override to create custom slider thumb shape


//void drawListBoxBackground(juce::Graphics& g, juce::ListBox& listBox) {
//    auto bounds = listBox.getLocalBounds().toFloat();
//
//    juce::Path clipPath;
//    clipPath.addRoundedRectangle(listBox.getLocalBounds().toFloat(), 10.0f);
//    // Draw a filled rounded rectangle as the background
//    //g.setColour(theme_()->getColorValue("ControlForeground"));
//    g.setColour(juce::Colours::red);
//    g.fillRoundedRectangle(bounds, 10.0f);
//
//    // Apply the clipping to the entire ListBox
//    g.reduceClipRegion(clipPath);
//    g.setColour(juce::Colours::blue.withAlpha(0.2f));
//    g.fillAll();

    // Set the color for the background
    //g.setColour(theme_()->getColorValue("ControlBackground"));
//    g.fillAll();
//}

//void drawListBoxItem(int rowNumber, juce::Graphics& g, int width, int height,
//                     bool rowIsSelected, const juce::String& text, juce::ListBox& listBox)
//    {
//        // Fill the entire width for the background, including the scrollbar area
//        if (rowIsSelected) {
//            g.fillAll(theme_()->getColorValue("SelectionBackground"));
//            g.setColour(theme_()->getColorValue("SelectionForeground"));
//        } else {
//            g.fillAll(theme_()->getColorValue("Background"));
//            g.setColour(theme_()->getColorValue("ControlForeground"));
//        }
//
//        // Draw the text
//        //g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
//    }

//void drawListBox(juce::Graphics& g, juce::ListBox& listBox,
//                 juce::BorderSize<int> border, int outlineThickness)
//{
//    // First, draw the list box background and items
//    drawListBox(g, listBox, border, outlineThickness);
//
//    // Now, draw our custom extended highlights
//    int itemHeight = listBox.getRowHeight();
//    int totalItems = listBox.getNumRows();
//    int firstVisibleIndex = listBox.getFirstRowOnScreen();
//    int numVisibleItems = listBox.getNumRowsOnScreen();
//
//    for (int i = firstVisibleIndex; i < firstVisibleIndex + numVisibleItems && i < totalItems; ++i)
//    {
//        if (listBox.isRowSelected(i))
//        {
//            // Calculate item position
//            int itemY = (i - firstVisibleIndex) * itemHeight;
//
//            // Draw extended highlight
//            g.setColour(highlightColor);
//            g.fillRect(0, itemY, listBox.getWidth(), itemHeight);
//        }
//    }
//}
//
