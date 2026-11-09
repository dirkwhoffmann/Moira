# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Moira'
copyright = '2025, Dirk W. Hoffmann'
author = 'Dirk W. Hoffmann'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.githubpages',
    'myst_parser'
]


templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']
source_parsers = {
    '.md': 'recommonmark.parser.CommonMarkParser',
}
source_suffix = ['.rst', '.md']

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

#html_theme = 'karma_sphinx_theme'
#html_theme = 'sphinx_rtd_theme'
html_theme = 'sphinx_book_theme'
html_theme_options = {
    "repository_url": "https://dirkwhoffmann.github.io/Moira/",
    "repository_provider": "github",
    "use_repository_button": True,
    "use_issues_button": False,
    "logo": {
        "text": "Moira v3.0",
        "image_light": "images/logo.png",
        "image_dark": "images/logo.png",
    }
}
# html_logo = "images/logo.png"
html_title = "Moira v3.0"
html_static_path = ["_static"]
html_css_files = ["custom.css"]
