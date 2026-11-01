RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"

COLORS = {
    "gray": "\033[38;5;245m",
    "green": "\033[38;5;46m",
    "dark_green": "\033[38;5;22m",
    "gold": "\033[38;5;178m",
    "cyan": "\033[38;5;51m",
    "blue": "\033[38;5;33m",
    "red": "\033[38;5;196m",
    "purple": "\033[38;5;141m",
    "orange": "\033[38;5;202m",
}

def c(text, color, bold=False, dim=False):
    prefix = ""
    if bold:
        prefix += BOLD
    if dim:
        prefix += DIM
    prefix += COLORS[color]
    return prefix + text + RESET
