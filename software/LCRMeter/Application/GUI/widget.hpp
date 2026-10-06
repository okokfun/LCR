#ifndef GUI_WIDGET_H_
#define GUI_WIDGET_H_

#include <string.h>
#include <stdint.h>
#include <util.h>
#include "events.hpp"

class Widget {
    /* 能够包含子节点的类，需要对其子节点的成员拥有扩展的访问权限 */
    friend class Container;
    friend class Window;
    friend class Menu;
    friend void guiThread(void);
  public:
    enum class Type : uint8_t {
        Button,
        Checkbox,
        Container,
        Custom,
        Entry,
        Graph,
        ItemChooser,
        Keyboard,
        Label,
        Progressbar,
        Sevensegment,
        Textfield,
        Widget,
        Window,
        Scopescreen,
        Menu,
        MenuBool,
        MenuBack,
        MenuChooser,
        EventCatcher,
        MenuValue,
        MenuAction,
        Desktop,
        Radiobutton,
        Slider,
    };

    Widget();
    virtual ~Widget();

    using Callback = void (*)(void*, Widget*);

    static void draw(Widget* w, coords_t pos);
    static void input(Widget* w, GUIEvent_t* ev);

    static Widget* getSelected() {
        return selectedWidget;
    }

    static void deselect();
    void select(bool down = true);

    void requestRedrawChildren();
    void requestRedraw();
    void requestRedrawFull();

    void setSelectable(bool s) {
        if (s && !selectable) {
            selectable = true;
            requestRedrawFull();
        } else if (!s && selectable) {
            selectable = false;
            requestRedrawFull();
        }
    }

    void setVisible(bool v) {
        if (visible != v) {
            visible = v;
            requestRedrawFull();
        }
    }

    bool isInArea(coords_t pos);

    coords_t getSize() {
        return size;
    }

    void setPosition(coords_t pos) {
        position = pos;
    }

    void setParent(Widget* p) {
        this->parent = p;
    }

    void addChild(Widget* w, coords_t pos);

  protected:
    Widget* IntSelectChild();
    Widget* GetNth(uint16_t n);

    virtual void draw(coords_t offset) {
        return;
    }
    virtual void input(GUIEvent_t* ev) {
        return;
    }
    virtual void drawChildren(coords_t offset) {
        return;
    }
    virtual Type getType() = 0;

    static Widget* selectedWidget;

    Widget* parent;
    Widget* firstChild;
    Widget* next;

    coords_t position;
    coords_t size;
    bool visible : 1;
    bool selected : 1;
    bool selectable : 1;
    //	bool focus :1;
    /* 该控件需要重绘 */
    bool redraw : 1;
    /* 该控件区域必须被清空，并且整个控件需要完全重绘 */
    bool redrawClear : 1;
    /* 该控件分支下的某个子控件需要进行重绘 */
    bool redrawChild : 1;
};

#endif
