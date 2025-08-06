#ifndef KEYBOARD_EVENTS_H
#define KEYBOARD_EVENTS_H

// Represents a single keyboard event.
typedef struct {
    enum {
        KEY_PRESS,
        KEY_RELEASE
    } type;

    // The raw scancode from the keyboard.
    unsigned char scancode;

    // If the key corresponds to a printable ASCII character, it will be stored here.
    // Otherwise, this will be 0.
    char ascii;

    // Flags for modifier keys.
    struct {
        bool shift;
        bool ctrl;
        bool alt;
    } modifiers;

    // For special, non-printable keys.
    enum {
        KEY_NONE = 0,
        KEY_UP_ARROW,
        KEY_DOWN_ARROW,
        KEY_LEFT_ARROW,
        KEY_RIGHT_ARROW,
        KEY_PAGE_UP,
        KEY_PAGE_DOWN,
        KEY_ESCAPE
    } special_key;

} keyboard_event_t;

#endif // KEYBOARD_EVENTS_H