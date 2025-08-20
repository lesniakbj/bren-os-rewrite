#include <drivers/keyboard.h>
#include <arch/i386/io.h>

static keyboard_event_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static kuint8_t keyboard_buffer_head = 0;
static kuint8_t keyboard_buffer_tail = 0;

static const char scancode_map_set_1[0x59] = {
    /*0x00*/  0,
    /*0x01*/  0,   // Escape scanned, no printable char
    /*0x02*/  '1', /*0x03*/ '2', /*0x04*/ '3', /*0x05*/ '4',
    /*0x06*/  '5', /*0x07*/ '6', /*0x08*/ '7', /*0x09*/ '8',
    /*0x0A*/  '9', /*0x0B*/ '0', /*0x0C*/ '-', /*0x0D*/ '=',
    /*0x0E*/ '\b', /*0x0F*/ '\t',
    /*0x10*/  'Q', /*0x11*/ 'W', /*0x12*/ 'E', /*0x13*/ 'R',
    /*0x14*/  'T', /*0x15*/ 'Y', /*0x16*/ 'U', /*0x17*/ 'I',
    /*0x18*/  'O', /*0x19*/ 'P', /*0x1A*/ '[', /*0x1B*/ ']',
    /*0x1C*/ '\n',
    /*0x1D*/  0,   // Left Ctrl
    /*0x1E*/  'A', /*0x1F*/ 'S', /*0x20*/  'D', /*0x21*/ 'F',
    /*0x22*/  'G', /*0x23*/ 'H', /*0x24*/  'J', /*0x25*/ 'K', /*0x26*/ 'L',
    /*0x27*/  ';', /*0x28*/ '\'', /*0x29*/ '`',
    /*0x2A*/  0,   // Left Shift
    /*0x2B*/ '\\', /*0x2C*/ 'Z', /*0x2D*/ 'X',
    /*0x2E*/  'C', /*0x2F*/ 'V', /*0x30*/ 'B', /*0x31*/ 'N',
    /*0x32*/  'M', /*0x33*/ ',', /*0x34*/ '.', /*0x35*/ '/',
    /*0x36*/  0,   // Right Shift
    /*0x37*/  '*', // (keypad) * pressed
    /*0x38*/  0,   // Left Alt
    /*0x39*/  ' ', // Spacebar
    /*0x3A*/  0,   // CAPSLOCK
    /*0x3B*/  0,   // F1
    /*0x3C*/  0,   // F2
    /*0x3D*/  0,   // F3
    /*0x3E*/  0,   // F4
    /*0x3F*/  0,   // F5
    /*0x40*/  0,   // F6
    /*0x41*/  0,   // F7
    /*0x42*/  0,   // F8
    /*0x43*/  0,   // F9
    /*0x44*/  0,   // F10
    /*0x45*/  0,   // Num Lock
    /*0x46*/  0,   // Scroll Lock
    /*0x47*/  0,   // keypad 7/Home
    /*0x48*/  0,   // keypad 8/Up
    /*0x49*/  0,   // keypad 9/PgUp
    /*0x4A*/  0,   // keypad -
    /*0x4B*/  0,   // keypad 4/Left
    /*0x4C*/  0,   // keypad 5
    /*0x4D*/  0,   // keypad 6/Right
    /*0x4E*/  0,   // keypad +
    /*0x4F*/  0,   // keypad 1/End
    /*0x50*/  0,   // keypad 2/Down
    /*0x51*/  0,   // keypad 3/PgDn
    /*0x52*/  0,   // keypad 0/Insert
    /*0x53*/  0,   // keypad ./Delete
    /*0x54*/  0,   // keypad .
    /*0x57*/  0,   // (various international keys)
    /*0x58*/  0,   // F11
    /*0x59*/  0    // F12
};

// Helper to enqueue the next event for the keyboard, this uses the circular buffer and rotates around it
static void enqueue_event(keyboard_event_t event) {
    kint32_t next = (keyboard_buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != keyboard_buffer_tail) {
        keyboard_buffer[keyboard_buffer_head] = event;
        keyboard_buffer_head = next;
    }
}

void keyboard_init() {
    keyboard_buffer_head = 0;
    keyboard_buffer_tail = 0;
}

// Function to return a keyboard event if there are any present
bool keyboard_poll(keyboard_event_t *out_event) {
    // We have events if the tail and head are not equal. Get the event and then move the tail
    if (keyboard_buffer_tail != keyboard_buffer_head) {
        *out_event = keyboard_buffer[keyboard_buffer_tail];
        keyboard_buffer_tail = (keyboard_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
        return true;
    }
    return false;
}

// Interrupt Keyboard handler
void keyboard_handler(registers_t *regs) {
    // Read and check the command scancode. If its an ACK or extended code, return.
    static bool is_extended = false;
    kuint8_t scancode = inb(KEYBOARD_READ_PORT);
    keyboard_event_t event = {0};
    if(scancode == KEYBOARD_CMD_ACK_CODE) {
        return;
    }
    if (scancode == KEYBOARD_EXTENDED_CODE) {
        is_extended = true;
        return;
    }

    // ... Otherwise lets start to decode the scancode
    event.type = (scancode & KEYBOARD_RELEASE_MASK) ? KEY_RELEASE : KEY_PRESS;
    event.scancode = scancode;
    if (is_extended) {
        // Handle extended scancodes here (Key Up, Down, etc)
        switch (scancode) {
            case 0x48: event.special_key = KEY_UP_ARROW; break;
            case 0x50: event.special_key = KEY_DOWN_ARROW; break;
            case 0x4B: event.special_key = KEY_LEFT_ARROW; break;
            case 0x4D: event.special_key = KEY_RIGHT_ARROW; break;
            case 0x49: event.special_key = KEY_PAGE_UP; break;
            case 0x51: event.special_key = KEY_PAGE_DOWN; break;
        }
        is_extended = false;
    } else {
        // Otherwise use our static scancode map to translate the scancodes
        size_t size = sizeof(scancode_map_set_1);
        if(event.type == KEY_PRESS && scancode < size) {
            event.ascii = scancode_map_set_1[scancode];
        } else if(event.type == KEY_RELEASE && (size_t)(scancode - KEYBOARD_RELEASE_MASK) < size) {
            event.ascii = scancode_map_set_1[scancode - KEYBOARD_RELEASE_MASK];
        }

        if (scancode == 0x01) {
            event.special_key = KEY_ESCAPE;
        }
    }

    // Enqueue this event into the circular buffer
    enqueue_event(event);
    return;
}