#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <algorithm>

using namespace std;

Display *d = nullptr;
Window root;
const char *HIDDEN_FILE = "/tmp/cotray_hidden_windows";

string get_window_name(Window w, Atom a) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = nullptr;
    if (XGetWindowProperty(d, w, a, 0, (~0L), False, AnyPropertyType,
                           &actual_type, &actual_format,
                           &nitems, &bytes_after, &prop) == Success) {
        if (prop) {
            string result = string(reinterpret_cast<char *>(prop));
            XFree(prop);
            return result;
        }
    }
    return "";
}

bool is_visible(Window w) {
    XWindowAttributes xwa;
    return XGetWindowAttributes(d, w, &xwa) && xwa.map_state == IsViewable;
}

vector<Window> get_top_windows() {
    Window parent;
    Window *children;
    unsigned int nchildren;
    vector<Window> result;
    if (XQueryTree(d, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; ++i) {
            result.push_back(children[i]);
        }
        if (children) XFree(children);
    }
    return result;
}

vector<Window> read_history() {
    vector<Window> hidden;
    ifstream in(HIDDEN_FILE);
    string line;
    while (getline(in, line)) {
        if (!line.empty()) {
            Window w = strtol(line.c_str(), nullptr, 16);
            hidden.push_back(w);
        }
    }
    return hidden;
}

void write_history(const vector<Window>& hidden) {
    ofstream out(HIDDEN_FILE, ios::trunc);
    for (Window w : hidden) {
        out << hex << w << "\n";
    }
}

bool add_to_history(Window w) {
    vector<Window> hidden = read_history();
    for (Window h : hidden) {
        if (h == w) return false;
    }
    hidden.push_back(w);
    write_history(hidden);
    return true;
}

bool remove_from_history(Window w) {
    vector<Window> hidden = read_history();
    auto it = remove(hidden.begin(), hidden.end(), w);
    if (it == hidden.end()) return false;
    hidden.erase(it, hidden.end());
    write_history(hidden);
    return true;
}

void show_window(Window w) {
    if (remove_from_history(w))
        cout << "[cotray] Window 0x" << hex << w << " removed from history and shown\n";
    else
        cout << "[cotray] Window 0x" << hex << w << " was not in hidden history\n";
    XMapWindow(d, w);
    XFlush(d);
}

void hide_window(Window w) {
    if (add_to_history(w))
        cout << "[cotray] Window 0x" << hex << w << " added to history and hidden\n";
    else
        cout << "[cotray] Window 0x" << hex << w << " is already hidden\n";
    XUnmapWindow(d, w);
    XFlush(d);
}

void print_help() {
    cout <<
    "cotray - hide/show windows from tray\n\n"
    "Usage:\n"
    "  cotray            List visible windows\n"
    "  cotray hide <win> Show window <win> (hex id)\n"
    "  cotray show <win> Hide window <win> (hex id)\n"
    "  cotray restore    Show all hidden windows\n"
    "  cotray list       List hidden windows\n"
    "  cotray clear      Clear hidden windows history\n"
    "  cotray help       Show this help message\n";
}

int main(int argc, char **argv) {
    d = XOpenDisplay(nullptr);
    if (!d) {
        cerr << "Failed to open X display\n";
        return 1;
    }
    root = DefaultRootWindow(d);

    if (argc == 1) {
        vector<Window> windows = get_top_windows();
        for (Window w : windows) {
            string name = get_window_name(w, XInternAtom(d, "_NET_WM_NAME", False));
            if (name.empty()) name = get_window_name(w, XInternAtom(d, "WM_NAME", False));
            if (name.empty() || !is_visible(w)) continue;
            cout << "0x" << hex << w << " \t" << name << endl;
        }
    } else if (argc == 3 && strcmp(argv[1], "show") == 0) {
        Window w = strtol(argv[2], nullptr, 0);
        show_window(w);
    } else if (argc == 3 && strcmp(argv[1], "hide") == 0) {
        Window w = strtol(argv[2], nullptr, 0);
        hide_window(w);
    } else if (argc == 2 && strcmp(argv[1], "restore") == 0) {
        vector<Window> hidden = read_history();
        for (Window w : hidden) {
            show_window(w);
        }
        write_history({}); // clear history after restore
        cout << "[cotray] Restored all hidden windows and cleared history\n";
    } else if (argc == 2 && strcmp(argv[1], "list") == 0) {
        vector<Window> hidden = read_history();
        if (hidden.empty()) cout << "[cotray] No hidden windows\n";
        for (Window w : hidden) {
            string name = get_window_name(w, XInternAtom(d, "_NET_WM_NAME", False));
            if (name.empty()) name = get_window_name(w, XInternAtom(d, "WM_NAME", False));
            cout << "0x" << hex << w << " \t" << name << endl;
        }
    } else if (argc == 2 && strcmp(argv[1], "clear") == 0) {
        write_history({});
        cout << "[cotray] Cleared hidden windows history\n";
    } else if (argc == 2 && strcmp(argv[1], "help") == 0) {
        print_help();
    } else {
        cout << "[cotray] Unknown command. Use 'cotray help' for usage.\n";
    }

    XCloseDisplay(d);
    return 0;
}
