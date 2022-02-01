#include "interfaceutils.h"

namespace InterfaceUtils
{

bool isDarkMode() {
    // ubuntu default menubar color is black
    // no matter the settings > appearance > window colors selection (light, standard, dark)
    // In future we may want to consider manually changed menubar color with gnome tweak or manual editing OS css file
    return true;
}

} // InterfaceUtils
