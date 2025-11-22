import os
import subprocess
import graphviz
import base64
from dash import Dash, dcc, html, Input, Output, State, callback, ctx
import dash_bootstrap_components as dbc

# Directory where .dot files will be generated
DOT_DIR = "./cfg_images"

# Ensure directory exists
os.makedirs(DOT_DIR, exist_ok=True)

import re
def get_dot_files():
    def extract_number(filename):
        m = re.search(r'(\d+)\.dot$', filename)
        return int(m.group(1)) if m else -1

    return sorted(
        [f for f in os.listdir(DOT_DIR) if f.endswith(".dot")],
        key=extract_number
    )

def dot_to_png_base64(dot_file_path):
    try:
        with open(dot_file_path, 'r') as f:
            dot_source = f.read()
        src = graphviz.Source(dot_source, engine='dot')
        png_bytes = src.pipe(format='png')
        return f'data:image/png;base64,{base64.b64encode(png_bytes).decode()}'
    except Exception as e:
        return None


app = Dash(__name__, external_stylesheets=[dbc.themes.BOOTSTRAP])

app.layout = dbc.Container([
    html.H1("LLVM C Code CFG & Live Variable Per Iteration", className="my-4 text-center"),
    
    dbc.Button("Show / Hide Instructions", id="toggle-instructions-btn", color="secondary", className="mb-3"),

    dbc.Collapse(
        dbc.Card(
            dbc.CardBody([
                html.H5("Instructions"),
                html.P("""Simple C codes can be input. Loops and if-else control structures are supported. All variables should be input at the beginning of the program.""")
            ]),
            className="mb-4 shadow-sm",
        ),
        id="instructions-collapse",
        is_open=False
    ),

    dbc.Row([
    # ---- LEFT SIDE: C CODE TEXTBOX ----
    dbc.Col([
        html.Label("Enter C Code:", style={"fontWeight": "bold"}),
        dcc.Textarea(
            id="c-code-input",
            value="""#include <stdio.h>
int main() {
    int x = 3;
    int y = 4;
    int z = 8;
    if (x < z) {
      y = 4;
      printf("%d", z);
    }
    if (x < y) {
        if (y > 2) {
            x = y + 1;
        } else {
            x = 0;
        }
    }
    return x;
}
""",
            style={"width": "100%", "height": "350px", "fontFamily": "monospace"}
        )
    ], width=8),


    # ---- RIGHT SIDE: CLANG FLAGS + INSTRUCTIONS ----
    dbc.Col([
        html.Label("Clang Compile Flags:", style={"fontWeight": "bold", "marginTop": "5px"}),
        dcc.Input(
            id="clang-args-input",
            type="text",
            value="-O0",
            debounce=True,
            style={"width": "100%", "marginBottom": "20px"}
        )
    ], width=4),
], className="mb-4"),

    dbc.Button("Generate CFG", id="generate-btn", color="primary", className="my-3"),

    html.Hr(),

    dbc.Card(
    dbc.CardBody([
        dbc.Row([
            dbc.Col(
                dbc.Button("⟨ Prev", id="prev-btn", color="info", className="w-100"),
                width=2
            ),
            dbc.Col(
                dcc.Dropdown(
                    id="file-dropdown",
                    options=[{'label': f, 'value': f} for f in get_dot_files()],
                    value=get_dot_files()[0] if get_dot_files() else None,
                    clearable=False,
                    style={"color": "black"}  # required for dark themes
                ),
                width=8
            ),
            dbc.Col(
                dbc.Button("Next ⟩", id="next-btn", color="info", className="w-100"),
                width=2
            ),
        ], align="center"),
    ]),
    className="mb-4 shadow"),

    dcc.Loading(
    html.Div(
        html.Img(
            id="png-image",
            style={
                'maxWidth': '90%',
                'maxHeight': '90vh',
                'height': 'auto',
                'margin': '20px auto',
                'display': 'block',
                'border': '2px solid #444',
                'borderRadius': '10px',
                'boxShadow': '0 0 15px rgba(0,0,0,0.5)'
            }
        ),
        style={"textAlign": "center"}
    )),

    dcc.Store(id="current-index", data=0),
    dcc.Store(id="file-list-store", data=get_dot_files()),
], fluid=True)


# ---------------- CFG GENERATION CALLBACK ----------------
@callback(
    Output("file-list-store", "data"),
    Output("file-dropdown", "value"),
    Input("generate-btn", "n_clicks"),
    State("c-code-input", "value"),
    State("clang-args-input", "value"),
    prevent_initial_call=True
)
def generate_cfg(n, c_code, clang_args):
    with open("input_temp.c", "w") as f:
        f.write(c_code)
    for filename in os.listdir("./cfg_images"):
        dot_file_path = os.path.join("./cfg_images", filename)
        os.remove(dot_file_path)
    try:
        subprocess.run(["clang-20", "-S", "-emit-llvm", "-g"] + clang_args.split() + ["input_temp.c", "-o", "input_temp.ll"], check=True)
        subprocess.run(["opt-20", "-load-pass-plugin", "./analysis/build/analysis/livePass.so", "-passes=live-analysis", "-disable-output", "input_temp.ll"], check=True)

    except Exception as e:
        print("Error:", e)

    files = get_dot_files()
    return files, (files[0] if files else None)

# --------------- Instructions callback -------------
@callback(
    Output("instructions-collapse", "is_open"),
    Input("toggle-instructions-btn", "n_clicks"),
    State("instructions-collapse", "is_open"),
    prevent_initial_call=True
)
def toggle_instructions(n, is_open):
    return not is_open

# ---------------- NAVIGATION CALLBACK ----------------
@callback(
    Output("current-index", "data"),
    Output("file-dropdown", "value", allow_duplicate=True),
    Input("prev-btn", "n_clicks"),
    Input("next-btn", "n_clicks"),
    Input("file-dropdown", "value"),
    State("current-index", "data"),
    State("file-list-store", "data"),
    prevent_initial_call=True
)
def navigate(prev, next, selected, index, files):
    if not files:
        return 0, None

    trigger = ctx.triggered_id
    if trigger == "prev-btn":
        index = (index - 1) % len(files)
    elif trigger == "next-btn":
        index = (index + 1) % len(files)
    elif trigger == "file-dropdown":
        index = files.index(selected)

    return index, files[index]

# ---------------- REFRESH DROPDOWN OPTIONS ----------------
@callback(
    Output("file-dropdown", "options"),
    Input("file-list-store", "data")
)
def refresh_dropdown_options(files):
    if not files:
        return []
    return [{'label': f, 'value': f} for f in files]

# ---------------- IMAGE DISPLAY CALLBACK ----------------
@callback(
    Output("png-image", "src"),
    Input("file-dropdown", "value")
)
def display_graph(selected):
    if not selected:
        return None
    path = os.path.join(DOT_DIR, selected)
    return dot_to_png_base64(path)


if __name__ == "__main__":
    app.run(debug=True)
