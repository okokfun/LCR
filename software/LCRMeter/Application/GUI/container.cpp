#include "container.hpp"
#include "log.h"

#include "Unit.hpp"

Container::Container(coords_t size) {
    this->size = size;
    editing = false;
    focussed = false;
    scrollHorizontal = false;
    scrollVertical = false;
    canvasSize.x = 0;
    canvasSize.y = 0;
    canvasOffset.x = 0;
    canvasOffset.y = 0;
    scrollBarLength.x = 0;
    scrollBarLength.y = 0;
    viewingSize = size;
}

void Container::attach(Widget* w, coords_t offset) {
    addChild(w, offset);
    /* 必要时扩展画布尺寸（或：按需调整画布大小） */
    if (canvasSize.x < w->position.x + w->size.x)
        canvasSize.x = w->position.x + w->size.x;
    if (canvasSize.y < w->position.y + w->size.y)
        canvasSize.y = w->position.y + w->size.y;
    /* add scroll bars if necessary */
    if (canvasSize.x > size.x) {
        scrollHorizontal = true;
        viewingSize.y = size.y - ScrollbarSize;
    }
    if (canvasSize.y > viewingSize.y) {
        scrollVertical = true;
        viewingSize.x = size.x - ScrollbarSize;
        if (!scrollHorizontal) {
            /* 重新检查是否需要水平滚动（或：再次检测水平滚动条） */
            if (canvasSize.x > viewingSize.x) {
                scrollHorizontal = true;
                viewingSize.y = size.y - ScrollbarSize;
            }
        }
    }
    /* 调整滚动条尺寸 */
    if (scrollHorizontal)
        scrollBarLength.x = util_Map(viewingSize.x, 0, canvasSize.x,
                                     0,
                                     viewingSize.x);
    if (scrollVertical)
        scrollBarLength.y = util_Map(viewingSize.y, 0, canvasSize.y,
                                     0,
                                     viewingSize.y);
}

void Container::draw(coords_t offset) {
    //	Widget *child = firstChild;
    //	Widget *selected = child;
    //	for (; selected; selected = selected->next) {
    //		if (selected->selected) {
    //			/* move canvas offset so that selected is visible */
    //			if (selected->position.x < canvasOffset.x) {
    //				canvasOffset.x = selected->position.x;
    //			} else if (selected->position.x + selected->size.x
    //					> viewingSize.x + canvasOffset.x) {
    //				canvasOffset.x = selected->position.x + selected->size.x
    //						- viewingSize.x;
    //			}
    //			if (selected->position.y < canvasOffset.y) {
    //				canvasOffset.y = selected->position.y;
    //			} else if (selected->position.y + selected->size.y
    //					> viewingSize.y + canvasOffset.y) {
    //				canvasOffset.y = selected->position.y + selected->size.y
    //						- viewingSize.y;
    //			}
    //		}
    //		break;
    //	}
    /* 必要时绘制滚动条 */
    if (scrollVertical) {
        display_SetForeground(LineColor);
        display_VerticalLine(offset.x + size.x - ScrollbarSize,
                             offset.y,
                             size.y);
        /* 计算滚动条的起始位置 */
        uint8_t scrollBegin = util_Map(canvasOffset.y, 0,
                                       canvasSize.y, 0,
                                       size.y - ScrollbarSize * scrollHorizontal);
        /* 显示位置指示器 */
        display_SetForeground(ScrollbarColor);
        display_RectangleFull(offset.x + size.x - ScrollbarSize + 1,
                              offset.y + scrollBegin, offset.x + size.x - 1,
                              offset.y + scrollBegin + scrollBarLength.y - 1);
    }
    if (scrollHorizontal) {
        display_SetForeground(LineColor);
        display_HorizontalLine(offset.x,
                               offset.y + size.y - ScrollbarSize,
                               size.x);
        /* 计算滚动条的起始位置 */
        uint8_t scrollBegin = util_Map(canvasOffset.x, 0,
                                       canvasSize.x, 0,
                                       size.x - ScrollbarSize * scrollVertical);
        /* 显示位置指示器 */
        display_SetForeground(ScrollbarColor);
        display_RectangleFull(offset.x + scrollBegin,
                              offset.y + size.y - ScrollbarSize + 1,
                              offset.x + scrollBegin + scrollBarLength.x - 1,
                              offset.y + size.y - 1);
    }
}

void Container::input(GUIEvent_t* ev) {
    switch (ev->type) {
        case EVENT_TOUCH_PRESSED:
        case EVENT_TOUCH_DRAGGED: {
                /* 保存旧的画布偏移量（记录拖动前的位置） */
                coords_t old = canvasOffset;
                if (ev->pos.x > viewingSize.x) {
                    /* 垂直滚动条 */
                    if (ev->type == EVENT_TOUCH_DRAGGED)
                        ev->pos.y = ev->dragged.y;
                    /* 调整垂直画布偏移量（上下滚动） */
                    canvasOffset.y = util_Map(ev->pos.y, scrollBarLength.y / 2,
                                              viewingSize.y - scrollBarLength.y / 2, 0,
                                              canvasSize.y - viewingSize.y);
                    /* 限制偏移量范围（防止把画布拖出边界） */
                    if (canvasOffset.y < 0)
                        canvasOffset.y = 0;
                    else if (canvasOffset.y > canvasSize.y - viewingSize.y)
                        canvasOffset.y = canvasSize.y - viewingSize.y;
                    /* 清除事件（处理完滚动后，把事件状态重置） */
                    ev->type = EVENT_NONE;
                } else if (ev->pos.y > size.y - scrollHorizontal*
                           ScrollbarSize) {
                    /* 水平滚动条 */
                    if (ev->type == EVENT_TOUCH_DRAGGED)
                        ev->pos.x = ev->dragged.x;
                    /* 调整水平画布偏移量（左右滚动） */
                    canvasOffset.x = util_Map(ev->pos.x, scrollBarLength.x / 2,
                                              viewingSize.x - scrollBarLength.x / 2, 0,
                                              canvasSize.x - viewingSize.x);
                    /* 限制偏移量范围（防止把画布拖出边界） */
                    if (canvasOffset.x < 0)
                        canvasOffset.x = 0;
                    else if (canvasOffset.x > canvasSize.x - viewingSize.x)
                        canvasOffset.x = canvasSize.x - viewingSize.x;
                    /* 清除事件（处理完滚动后，把事件状态重置） */
                    ev->type = EVENT_NONE;
                }
                /* check if canvas moved */
                if (canvasOffset.x != old.x || canvasOffset.y != old.y) {
                    /* request redraw */
                    requestRedrawFull();
                }
            }
        /* no break */
        case EVENT_TOUCH_RELEASED:
            /* adjust position to canvas offset */
            ev->pos.x += canvasOffset.x;
            ev->pos.y += canvasOffset.y;
            break;
        //	case EVENT_BUTTON_CLICKED:
        //		if(ev->button & BUTTON_ESC) {
        //			if(parent && parent->selectable) {
        //				parent->select(false);
        //			} else {
        //				deselect();
        //			}
        //		} else if (ev->button
        //				& (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN)) {
        //			if (!selectedWidget) {
        //				firstChild->select();
        //				ev->type = EVENT_NONE;
        //				break;
        //			} else if (selectedWidget->parent != this) {
        //				/* do nothing */
        //				break;
        //			} else {
        //				/* currently selected widget is child of this container. Iterate
        //				 * over all children and find the nearest one in the right direction.
        //				 * This child will be selected now */
        //				uint32_t minDist = UINT32_MAX;
        //				Widget *nearest = nullptr;
        //				Widget *child = firstChild;
        //				coords_t distFrom = selectedWidget->position;
        //				switch (ev->button) {
        //				case BUTTON_RIGHT:
        //					distFrom.x += selectedWidget->size.x;
        //				case BUTTON_LEFT:
        //					distFrom.y += selectedWidget->size.y / 2;
        //					break;
        //				case BUTTON_DOWN:
        //					distFrom.y += selectedWidget->size.y;
        //				case BUTTON_UP:
        //					distFrom.x += selectedWidget->size.x / 2;
        //					break;
        //				}
        //				for (; child; child = child->next) {
        //					if (child == selectedWidget || !child->visible
        //							|| !child->selectable) {
        //						/* This child is not eligible for selection */
        //						continue;
        //					}
        //					for (uint8_t i = 0; i < 4; i++) {
        //						/* iterate over all four corners of the widget */
        //						coords_t distTo = child->position;
        //						switch (i) {
        //						case 3:
        //							distTo.x += child->size.x;
        //							/* no break */
        //						case 2:
        //							distTo.y += child->size.y;
        //							break;
        //						case 1:
        //							distTo.x += child->size.x;
        //							break;
        //						}
        //						/* Skip if corner lies in the wrong direction */
        //						coords_t diff;
        //						diff.x = distTo.x - distFrom.x;
        //						diff.y = distTo.y - distFrom.y;
        //						if (ev->button == BUTTON_LEFT
        //								&& (diff.x > 0
        //										|| abs(diff.x)
        //												< abs(diff.y)
        //														- selectedWidget->size.y
        //																/ 2))
        //							continue;
        //						if (ev->button == BUTTON_RIGHT
        //								&& (diff.x < 0
        //										|| abs(diff.x)
        //												< abs(diff.y)
        //														- selectedWidget->size.y
        //																/ 2))
        //							continue;
        //						if (ev->button == BUTTON_UP
        //								&& (diff.y > 0
        //										|| abs(diff.y)
        //												< abs(diff.x)
        //														- selectedWidget->size.x
        //																/ 2))
        //							continue;
        //						if (ev->button == BUTTON_DOWN
        //								&& (diff.y < 0
        //										|| abs(diff.y)
        //												< abs(diff.x)
        //														- selectedWidget->size.x
        //																/ 2))
        //							continue;
        //						/* calculate distance */
        //						uint32_t dist = diff.x * diff.x + diff.y * diff.y;
        //						if (dist < minDist) {
        //							minDist = dist;
        //							nearest = child;
        //						}
        //					}
        //				}
        //				if (nearest != nullptr) {
        //					nearest->select();
        //					ev->type = EVENT_NONE;
        //				}
        //			}
        //		}
        //		break;
        default:
            break;
    }
}

void Container::drawChildren(coords_t offset) {
    Widget* child = firstChild;
    //    Widget *selected = child;
    //    for (; selected; selected = selected->next) {
    //        if (selected->selected)
    //            break;
    //    }
    display_SetActiveArea(offset.x, offset.x + viewingSize.x - 1,
                          offset.y,
                          offset.y + viewingSize.y - 1);
    offset.x -= canvasOffset.x;
    offset.y -= canvasOffset.y;
    /* draw its children */
    for (; child; child = child->next) {
        //		if (child->visible/* && !child->selected*/) {
        /* check if child is at least partially in viewing field */
        if (child->position.x + child->size.x >= canvasOffset.x
            && child->position.y + child->size.y >= canvasOffset.y
            && child->position.x <= canvasOffset.x + viewingSize.x
            && child->position.y <= canvasOffset.y + viewingSize.y) {
            /* draw this child */
            Widget::draw(child, offset);
        }
        //        }
    }
    //    /* always draw selected child last (might overwrite other children) */
    //    if (selected) {
    //    	Widget::draw(selected, offset);
    //    }
    display_SetDefaultArea();
}
