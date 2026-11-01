RESET = "\033[0m"
DIM   = "\033[2m"
BOLD  = "\033[1m"

FG_DIM_GREEN = "\033[38;5;22m"
FG_GREEN     = "\033[38;5;46m"
FG_SOFT      = "\033[38;5;108m"
FG_GRAY      = "\033[38;5;245m"
FG_GOLD      = "\033[38;5;178m"
FG_RED       = "\033[38;5;160m"

print(f"""{FG_DIM_GREEN}╔══════════════════════════════════════════════════════════╗
║{RESET}                                                          {FG_DIM_GREEN}║
║   {FG_GRAY}exploit status : {BOLD}{FG_GREEN}SUCCESS{RESET}{FG_DIM_GREEN}                               ║
║   {FG_GRAY}boundary       : {FG_SOFT}politely violated{FG_DIM_GREEN}                     ║
║   {FG_GRAY}stack canary   : {FG_GOLD}emotionally unavailable{FG_DIM_GREEN}               ║
║   {FG_GRAY}ASLR           : {FG_RED}went for a walk{FG_DIM_GREEN}                       ║
║                                                          ║
║              {FG_SOFT}⠀⠀⠀⠀⠀⠀⠀⢀⣀⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀{FG_DIM_GREEN}                         ║
║              {FG_SOFT}⠀⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣦⡀⠀⠀⠀⠀{FG_DIM_GREEN}                          ║
║              {FG_SOFT}⠀⠀⠀⣰⣿⣿⠟⠉⠉⠉⠛⣿⣿⣆⠀⠀⠀{FG_DIM_GREEN}                           ║
║              {FG_SOFT}⠀⠀⢰⣿⣿⠁  {FG_GOLD}0xDEADBEEF{FG_SOFT}  ⣿⣿⡆⠀⠀{FG_DIM_GREEN}                   ║
║              {FG_SOFT}⠀⠀⢸⣿⣿⡀   {BOLD}{FG_GREEN}SATORI{RESET}{FG_SOFT}    ⣿⣿⡇⠀⠀{FG_DIM_GREEN}                    ║
║              {FG_SOFT}⠀⠀⠀⢿⣿⣿⣦⣀⣀⣀⣴⣿⣿⡿⠀⠀⠀{FG_DIM_GREEN}                           ║
║              {FG_SOFT}⠀⠀⠀⠀⠙⠿⣿⣿⣿⣿⣿⠿⠋⠀⠀⠀⠀{FG_DIM_GREEN}                           ║
║                                                          ║
║        {FG_GRAY}root shell acquired. {DIM}ego shell dropped.{RESET}{FG_DIM_GREEN}           ║
║                                                          ║
╚══════════════════════════════════════════════════════════╝{RESET}""")
