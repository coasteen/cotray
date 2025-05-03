#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <fstream>

using namespace std;

Display *d = nullptr;
Window root;
const char *HIDDEN_FILE = "/tmp/cotray_last_hidden";

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

void show_window(Window w) {
    XMapWindow(d, w);
    XFlush(d);
}

void hide_window(Window w) {
    ofstream out(HIDDEN_FILE);
    out << hex << w;
    out.close();
    XUnmapWindow(d, w);
    XFlush(d);
}

Window get_last_hidden() {
    ifstream in(HIDDEN_FILE);
    Window w = 0;
    in >> hex >> w;
    in.close();
    return w;
}

int main(int argc, char **argv) {
    d = XOpenDisplay(nullptr);
    if (!d) return 1;
    root = DefaultRootWindow(d);

    if (argc == 3 && strcmp(argv[1], "show") == 0) {
        Window w = strtol(argv[2], nullptr, 0);
        show_window(w);
        XCloseDisplay(d);
        return 0;
    } else if (argc == 3 && strcmp(argv[1], "hide") == 0) {
        Window w = strtol(argv[2], nullptr, 0);
        hide_window(w);
        XCloseDisplay(d);
        return 0;
    } else if (argc == 2 && strcmp(argv[1], "restore") == 0) {
        Window w = get_last_hidden();
        if (w) show_window(w);
        XCloseDisplay(d);
        return 0;
    }

    vector<Window> windows = get_top_windows();

    for (Window w : windows) {
        string name = get_window_name(w, XInternAtom(d, "_NET_WM_NAME", False));
        if (name.empty()) {
            name = get_window_name(w, XInternAtom(d, "WM_NAME", False));
        }
        if (name.empty() || !is_visible(w)) continue;

        cout << "0x" << hex << w << " \t" << name << endl;
    }

    XCloseDisplay(d);
    return 0;
}
