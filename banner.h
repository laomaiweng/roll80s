 /* figlet -f ascii12 <<<ARS2000 | sed -e 's/"/\\"/g; s/^/    "/; s/$/\\n"/' */
const char banner_bw[] =
    "                                                                      \n"
    "    mm     mmmmmm      mmmm     mmmmm      mmmm      mmmm      mmmm   \n"
    "   ####    ##\"\"\"\"##  m#\"\"\"\"#   #\"\"\"\"##m   ##\"\"##    ##\"\"##    ##\"\"##  \n"
    "   ####    ##    ##  ##m             ##  ##    ##  ##    ##  ##    ## \n"
    "  ##  ##   #######    \"####m       m#\"   ## ## ##  ## ## ##  ## ## ## \n"
    "  ######   ##  \"##m       \"##    m#\"     ##    ##  ##    ##  ##    ## \n"
    " m##  ##m  ##    ##  #mmmmm#\"  m##mmmmm   ##mm##    ##mm##    ##mm##  \n"
    " \"\"    \"\"  \"\"    \"\"\"  \"\"\"\"\"    \"\"\"\"\"\"\"\"    \"\"\"\"      \"\"\"\"      \"\"\"\"   \n"
    "                                                                      \n"
    "                                                                      \n"
;

 /* toilet --gay -f ascii12 <<<ARS2000 | sed -e 's/"/\\"/g; s/^/    "/; s/$/\\n"/' */
const char banner_color[] =
    "                                                                      \n"
    "    [0;1;32;92mmm[0m     [0;1;35;95mm[0;1;31;91mmm[0;1;33;93mmm[0;1;32;92mm[0m      [0;1;35;95mm[0;1;31;91mmm[0;1;33;93mm[0m     [0;1;34;94mmm[0;1;35;95mmm[0;1;31;91mm[0m      [0;1;36;96mm[0;1;34;94mmm[0;1;35;95mm[0m      [0;1;32;92mm[0;1;36;96mmm[0;1;34;94mm[0m      [0;1;33;93mm[0;1;32;92mmm[0;1;36;96mm[0m   \n"
    "   [0;1;32;92m#[0;1;36;96m##[0;1;34;94m#[0m    [0;1;31;91m#[0;1;33;93m#\"[0;1;32;92m\"\"[0;1;36;96m\"#[0;1;34;94m#[0m  [0;1;35;95mm[0;1;31;91m#\"[0;1;33;93m\"\"[0;1;32;92m\"#[0m   [0;1;34;94m#[0;1;35;95m\"\"[0;1;31;91m\"\"[0;1;33;93m##[0;1;32;92mm[0m   [0;1;34;94m##[0;1;35;95m\"\"[0;1;31;91m##[0m    [0;1;36;96m##[0;1;34;94m\"\"[0;1;35;95m##[0m    [0;1;32;92m##[0;1;36;96m\"\"[0;1;34;94m##[0m  \n"
    "   [0;1;36;96m#[0;1;34;94m##[0;1;35;95m#[0m    [0;1;33;93m#[0;1;32;92m#[0m    [0;1;34;94m#[0;1;35;95m#[0m  [0;1;31;91m#[0;1;33;93m#m[0m             [0;1;32;92m#[0;1;36;96m#[0m  [0;1;34;94m#[0;1;35;95m#[0m    [0;1;33;93m#[0;1;32;92m#[0m  [0;1;36;96m#[0;1;34;94m#[0m    [0;1;31;91m#[0;1;33;93m#[0m  [0;1;32;92m#[0;1;36;96m#[0m    [0;1;35;95m#[0;1;31;91m#[0m \n"
    "  [0;1;34;94m##[0m  [0;1;31;91m##[0m   [0;1;32;92m#[0;1;36;96m##[0;1;34;94m##[0;1;35;95m##[0m    [0;1;32;92m\"#[0;1;36;96m##[0;1;34;94m#m[0m       [0;1;32;92mm[0;1;36;96m#\"[0m   [0;1;35;95m#[0;1;31;91m#[0m [0;1;33;93m##[0m [0;1;32;92m#[0;1;36;96m#[0m  [0;1;34;94m#[0;1;35;95m#[0m [0;1;31;91m##[0m [0;1;33;93m#[0;1;32;92m#[0m  [0;1;36;96m#[0;1;34;94m#[0m [0;1;35;95m##[0m [0;1;31;91m#[0;1;33;93m#[0m \n"
    "  [0;1;35;95m##[0;1;31;91m##[0;1;33;93m##[0m   [0;1;36;96m#[0;1;34;94m#[0m  [0;1;35;95m\"[0;1;31;91m##[0;1;33;93mm[0m       [0;1;35;95m\"#[0;1;31;91m#[0m    [0;1;32;92mm[0;1;36;96m#\"[0m     [0;1;31;91m#[0;1;33;93m#[0m    [0;1;36;96m#[0;1;34;94m#[0m  [0;1;35;95m#[0;1;31;91m#[0m    [0;1;32;92m#[0;1;36;96m#[0m  [0;1;34;94m#[0;1;35;95m#[0m    [0;1;33;93m#[0;1;32;92m#[0m \n"
    " [0;1;35;95mm[0;1;31;91m##[0m  [0;1;32;92m##[0;1;36;96mm[0m  [0;1;34;94m#[0;1;35;95m#[0m    [0;1;33;93m#[0;1;32;92m#[0m  [0;1;36;96m#[0;1;34;94mmm[0;1;35;95mmm[0;1;31;91mm#[0;1;33;93m\"[0m  [0;1;32;92mm[0;1;36;96m##[0;1;34;94mmm[0;1;35;95mmm[0;1;31;91mm[0m   [0;1;32;92m##[0;1;36;96mmm[0;1;34;94m##[0m    [0;1;33;93m##[0;1;32;92mmm[0;1;36;96m##[0m    [0;1;31;91m##[0;1;33;93mmm[0;1;32;92m##[0m  \n"
    " [0;1;31;91m\"[0;1;33;93m\"[0m    [0;1;36;96m\"[0;1;34;94m\"[0m  [0;1;35;95m\"[0;1;31;91m\"[0m    [0;1;32;92m\"[0;1;36;96m\"\"[0m  [0;1;35;95m\"\"[0;1;31;91m\"\"[0;1;33;93m\"[0m    [0;1;36;96m\"[0;1;34;94m\"\"[0;1;35;95m\"\"[0;1;31;91m\"\"[0;1;33;93m\"[0m    [0;1;36;96m\"[0;1;34;94m\"\"[0;1;35;95m\"[0m      [0;1;32;92m\"[0;1;36;96m\"\"[0;1;34;94m\"[0m      [0;1;33;93m\"[0;1;32;92m\"\"[0;1;36;96m\"[0m   \n"
    "                                                                      \n"
    "                                                                      \n"
;
