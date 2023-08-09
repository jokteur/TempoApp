#include "keyboard_shortcuts.h"

#include <iostream>
#include "imgui.h"
//#include <clocale>

namespace Tempo {

    std::deque<KeyEvent> KeyboardShortCut::keyboard_events_;
    std::vector<Shortcut> KeyboardShortCut::global_shortcuts_;
    std::vector<Shortcut> KeyboardShortCut::local_shortcuts_;
    EventQueue& KeyboardShortCut::eventQueue_ = EventQueue::getInstance();
    bool KeyboardShortCut::ignore_global_shortcuts_ = false;
    std::mutex KeyboardShortCut::mutex;

    /*
     * Character utilities
     */
     // All kp keys that could get a name from glfwGetKeyName
    std::set<int> KeyboardShortCut::kp_keys_list_ = {
            GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3,
            GLFW_KEY_KP_4, GLFW_KEY_KP_5, GLFW_KEY_KP_6, GLFW_KEY_KP_7,
            GLFW_KEY_KP_8, GLFW_KEY_KP_9, GLFW_KEY_KP_DECIMAL,
            GLFW_KEY_KP_DIVIDE, GLFW_KEY_KP_MULTIPLY,
            GLFW_KEY_KP_SUBTRACT, GLFW_KEY_KP_ADD,
            GLFW_KEY_KP_EQUAL
    };

    // All kp keys that could get a name from glfwGetKeyName
    // std::set<char> KeyboardShortCut::authorized_chars_ = {
    //        '\'', ',', '-', '.', '/', ';', '=', '[', ']', '\\',
    //        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J','K', 'L',
    //        'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    //        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
    // };

    // Based on stb_to_utf8() from github.com/nothings/stb/
    static inline int charToUtf8(char* buf, int buf_size, unsigned int c)
    {
        if (c < 0x80)
        {
            buf[0] = (char)c;
            return 1;
        }
        if (c < 0x800)
        {
            if (buf_size < 2) return 0;
            buf[0] = (char)(0xc0 + (c >> 6));
            buf[1] = (char)(0x80 + (c & 0x3f));
            return 2;
        }
        if (c < 0x10000)
        {
            if (buf_size < 3) return 0;
            buf[0] = (char)(0xe0 + (c >> 12));
            buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
            buf[2] = (char)(0x80 + ((c) & 0x3f));
            return 3;
        }
        if (c <= 0x10FFFF)
        {
            if (buf_size < 4) return 0;
            buf[0] = (char)(0xf0 + (c >> 18));
            buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
            buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
            buf[3] = (char)(0x80 + ((c) & 0x3f));
            return 4;
        }
        // Invalid code point, the max unicode is 0x10FFFF
        return 0;
    }

    /*
     * Implementation of KeyboardShortCut
     */
    bool KeyboardShortCut::is_shortcut_valid(Shortcut& shortcut) {
        bool is_valid = false;

        timepoint previous_time;
        bool no_time_check = true;
        if (!keyboard_events_.empty()) {
            previous_time = keyboard_events_[0].time;
            no_time_check = false;
        }
        float time_between_keys = 0;

        // Once a shortcut is complete, it "consumes" the key events in the deque
        std::vector<std::deque<KeyEvent>::iterator> to_erase;

        for (auto it = keyboard_events_.begin(); it != keyboard_events_.end(); it++) {
            auto& event = *it;
            auto shortcut_iterator = find_key(shortcut, event.key);
            time_between_keys = (float)std::chrono::duration_cast<std::chrono::milliseconds>(previous_time - event.time).count();
            if (no_time_check)
                time_between_keys = 0;

            if (shortcut_iterator != shortcut.tmp_keys.end()
                && (event.state == GLFW_PRESS || time_between_keys < shortcut.delay)) {

                shortcut.tmp_keys.erase(shortcut_iterator);
                if (event.key != GLFW_KEY_LEFT_CONTROL && event.key != GLFW_KEY_RIGHT_CONTROL
                    && event.key != GLFW_KEY_LEFT_SUPER && event.key != GLFW_KEY_RIGHT_SUPER
                    && event.key != GLFW_KEY_LEFT_ALT && event.key != GLFW_KEY_RIGHT_ALT
                    && event.key != GLFW_KEY_LEFT_SHIFT && event.key != GLFW_KEY_RIGHT_SHIFT)
                    to_erase.push_back(it);

                // The shortcut is complete
                if (shortcut.tmp_keys.empty()) {
                    is_valid = true;
                    break;
                }

                // Only update the previous time of the key if the key was found in the shortcut
                previous_time = event.time;
            }
        }

        // Empty the queue until the last key event taken by the shortcut
        if (is_valid) {
            for (auto& it : to_erase)
                keyboard_events_.erase(it);
        }

        return is_valid;
    }

    void KeyboardShortCut::key_callback(GLFWwindow*, int key, int, int action, int) {
        std::lock_guard<std::mutex> guard(mutex);
        key = translate_keycode(key);
        if (action == GLFW_PRESS) {
            //last_keystroke_ = key;
            KeyEvent keyevent = { key, GLFW_PRESS, std::chrono::system_clock::now() };
            keyboard_events_.push_front(keyevent);

            if (keyboard_events_.size() > KEYBOARD_SHORTCUT_QUEUE_LENGTH)
                keyboard_events_.pop_back();
        }
        else if (action == GLFW_RELEASE) {
            for (auto& event : keyboard_events_) {
                if (key == event.key) {
                    event.state = GLFW_RELEASE;
                    break;
                }
            }
        }
    }

    void KeyboardShortCut::dispatchShortcuts() {
        std::lock_guard<std::mutex> guard(mutex);

        for (auto& shortcut : local_shortcuts_) {
            shortcut.tmp_keys = shortcut.keys;
            bool is_valid = is_shortcut_valid(shortcut);
            if (is_valid) {
                std::string event_name = std::string("shortcuts/local/") + shortcut.name;
                eventQueue_.post(Event_ptr(new Event(event_name)));
                if (shortcut.callback != NULL)
                    shortcut.callback();
            }

        }
        // Local shortcuts are only dispatched once, then they are destroyed
        local_shortcuts_.clear();
        if (ignore_global_shortcuts_) {
            ignore_global_shortcuts_ = false;
            return;
        }
        // Go through all global shortcuts
        for (auto& shortcut : global_shortcuts_) {
            shortcut.tmp_keys = shortcut.keys;
            bool is_valid = is_shortcut_valid(shortcut);
            if (is_valid) {
                std::string event_name = std::string("shortcuts/global/") + shortcut.name;
                eventQueue_.post(Event_ptr(new Event(event_name)));
                if (shortcut.callback != NULL)
                    shortcut.callback();
            }
        }
    }

    std::multiset<keyboard_event>::iterator KeyboardShortCut::find_key(Shortcut& shortcut, const int key) {
        for (auto it = shortcut.tmp_keys.begin();it != shortcut.tmp_keys.end();it++) {
            if (*it == key)
                return it;
            else if (*it == KEY_CTRL && (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL))
                return it;
            else if (*it == KEY_ALT && (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT))
                return it;
            else if (*it == KEY_SHIFT && (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT))
                return it;
            else if (*it == KEY_ENTER && (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER))
                return it;
            else if (*it == KEY_SUPER && (key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER))
                return it;
        }
        return shortcut.tmp_keys.end();
    }


    // Does not work with GLFW for now
    // TODO: use native API
    void KeyboardShortCut::character_callback(GLFWwindow*, unsigned int) {
        /*
       // Taken from imgui.h
        if (codepoint <= 127) {
            ImWchar c = codepoint > 0 && codepoint <= IM_UNICODE_CODEPOINT_MAX ? (ImWchar)codepoint : IM_UNICODE_CODEPOINT_INVALID;

            char *buf = new char[1];
            charToUtf8(buf, 1, c);

            char final_char = std::toupper(buf[0]);
            if(authorized_chars_.find(buf[0]) != authorized_chars_.end()) {

                delete[] buf;
            }
        }*/
    }

    int KeyboardShortCut::translate_keycode(int key) {
        const char* keyName = glfwGetKeyName(key, GLFW_KEY_UNKNOWN);

        // Register only non-named keys or numpad events
        if (keyName != NULL && kp_keys_list_.find(key) == kp_keys_list_.end()) {
            switch (keyName[0]) {
            case 'a':
                key = GLFW_KEY_A;
                break;
            case 'b':
                key = GLFW_KEY_B;
                break;
            case 'c':
                key = GLFW_KEY_C;
                break;
            case 'd':
                key = GLFW_KEY_D;
                break;
            case 'e':
                key = GLFW_KEY_E;
                break;
            case 'f':
                key = GLFW_KEY_F;
                break;
            case 'g':
                key = GLFW_KEY_G;
                break;
            case 'h':
                key = GLFW_KEY_H;
                break;
            case 'i':
                key = GLFW_KEY_I;
                break;
            case 'j':
                key = GLFW_KEY_J;
                break;
            case 'k':
                key = GLFW_KEY_K;
                break;
            case 'l':
                key = GLFW_KEY_L;
                break;
            case 'm':
                key = GLFW_KEY_M;
                break;
            case 'n':
                key = GLFW_KEY_N;
                break;
            case 'o':
                key = GLFW_KEY_O;
                break;
            case 'p':
                key = GLFW_KEY_P;
                break;
            case 'q':
                key = GLFW_KEY_Q;
                break;
            case 'r':
                key = GLFW_KEY_R;
                break;
            case 's':
                key = GLFW_KEY_S;
                break;
            case 't':
                key = GLFW_KEY_T;
                break;
            case 'u':
                key = GLFW_KEY_U;
                break;
            case 'v':
                key = GLFW_KEY_V;
                break;
            case 'w':
                key = GLFW_KEY_W;
                break;
            case 'x':
                key = GLFW_KEY_X;
                break;
            case 'y':
                key = GLFW_KEY_Y;
                break;
            case 'z':
                key = GLFW_KEY_Z;
                break;
            }
        }
        return key;
    }

    void KeyboardShortCut::addShortcut(Shortcut& shortcut) {
        global_shortcuts_.push_back(shortcut);
    }

    void KeyboardShortCut::emptyKeyEventsQueue() {
        while (!keyboard_events_.empty()) {
            keyboard_events_.pop_front();
        }

    }

    void KeyboardShortCut::addTempShortcut(Shortcut& shortcut) {
        local_shortcuts_.push_back(shortcut);
    }

    void KeyboardShortCut::flushTempShortcuts() {
        local_shortcuts_.clear();
    }
}