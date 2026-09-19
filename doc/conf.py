#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import subprocess

rtd_build = os.environ.get("READTHEDOCS", None) == "True"

if rtd_build:
    subprocess.call("doxygen --version; doxygen doxygen.conf", shell=True)

extensions = ["breathe", "sphinx.ext.todo"]

# Show the todo boxes in local builds only.
todo_include_todos = not rtd_build
templates_path = ["_templates"]
source_suffix = ".rst"
master_doc = "index"

project = "Ixion"
copyright = "2026, Kohei Yoshida"

version = "0.21"
release = "0.21.0"

exclude_patterns = ["_build"]

pygments_style = "sphinx"
html_theme = "sphinx_rtd_theme"

# theme-specific options
html_theme_options = {
    "navigation_depth": 2,
    "prev_next_buttons_location": "both",
    "style_external_links": False,
}

html_static_path = ["_static"]
html_css_files = ["custom.css"]
htmlhelp_basename = "ixiondoc"

latex_elements = {}
latex_documents = [
  ("index", "ixion.tex", "Ixion Documentation",
   "Kohei Yoshida", "manual"),
]

man_pages = [
    ("index", "ixion", "Ixion Documentation",
     ["Kohei Yoshida"], 1)
]

texinfo_documents = [
  ("index", "ixion", "Ixion Documentation",
   "Kohei Yoshida", "Ixion", "One line description of project.",
   "Miscellaneous"),
]

breathe_projects = {"ixion": "./_doxygen/xml"}
breathe_default_project = "ixion"
breathe_default_members = ("members", "undoc-members")

intersphinx_mapping = {
    "python": ("https://docs.python.org/3", None),
}

autodoc_member_order = "bysource"
