#ifndef WINDOW_PARAMETERS_H
#define WINDOW_PARAMETERS_H
#include <string>


const float scale_factor = 1/1.337142857f; //1/1.375;


//Sizes of a main window
const int width_window  = static_cast<int>(468*scale_factor);
const int height_window = static_cast<int>(470*scale_factor);
const int color_of_a_background=0x1C0D02;

const int logo_x=static_cast<int>(255*scale_factor);
const int logo_y=static_cast<int>(56*scale_factor);
const int logo_width=static_cast<int>(252*scale_factor);

const int install_windscribe_x = static_cast<int>(178*scale_factor);
const int install_windscribe_y = static_cast<int>(240*scale_factor);
const int install_windscribe_width  = static_cast<int>(114*scale_factor);
const int install_windscribe_height = static_cast<int>(44*scale_factor);

#ifdef _WIN32
const int install_windscribe_font_size = static_cast<int>(18*scale_factor);
const std::wstring install_windscribe_font = L"Bahnschrift Light";
const std::string install_windscribe_font1 = "Bahnschrift Light";
#else
const int install_windscribe_font_size = static_cast<int>(18*scale_factor);
const std::string install_windscribe_font = L"Chalkboard";
const std::string install_windscribe_font1 = "Chalkboard";
#endif


const int install_options_x = static_cast<int>(210*scale_factor);
const int install_options_y = static_cast<int>(370*scale_factor);
const int install_options_width  = static_cast<int>(50*scale_factor);
const int install_options_height = static_cast<int>(50*scale_factor);

const int install_options_font_size = static_cast<int>(26*scale_factor);
const std::wstring install_options_font = L"Arial";
const std::string install_options_font1 = "Arial";


const int button_edit_path_x = static_cast<int>(412*scale_factor);
const int button_edit_path_y = static_cast<int>(245*scale_factor);
const int button_edit_path_width = static_cast<int>(42*scale_factor);
const int button_edit_path_height = static_cast<int>(24*scale_factor);

const int edit_path_font_size = static_cast<int>(18*scale_factor);
const std::wstring edit_path_font = L"Bahnschrift Light";
const std::string edit_path_font1 = "Bahnschrift Light";

const int line_edit_path_x = static_cast<int>(37*scale_factor);
const int line_edit_path_y = static_cast<int>(291*scale_factor);

//const int text_edit_path_x = static_cast<int>(30*scale_factor);
//const int text_edit_path_y = static_cast<int>(265*scale_factor);

const int rect_edit_path_x1 = static_cast<int>(25*scale_factor);
const int rect_edit_path_y1 = static_cast<int>(245*scale_factor);
const int rect_edit_path_x2 = static_cast<int>(412*scale_factor);
const int rect_edit_path_y2 = static_cast<int>(280*scale_factor);


const int desktop_shortcut_x = static_cast<int>(412*scale_factor);
const int desktop_shortcut_y = static_cast<int>(315*scale_factor);
const int desktop_shortcut_width = static_cast<int>(41*scale_factor);
const int desktop_shortcut_height = static_cast<int>(22*scale_factor);

const int desktop_shortcut_font_size = static_cast<int>(18*scale_factor);
const std::wstring desktop_shortcut_font = L"Bahnschrift Light";
const std::string desktop_shortcut_font1 = "Bahnschrift Light";

const int line_desktop_shortcut_x = static_cast<int>(37*scale_factor);
const int line_desktop_shortcut_y = static_cast<int>(358*scale_factor);

//const int text_desktop_shortcut_x = static_cast<int>(30*scale_factor);
//const int text_desktop_shortcut_y = static_cast<int>(330*scale_factor);

const int rect_desktop_shortcut_x1 = static_cast<int>(25*scale_factor);
const int rect_desktop_shortcut_y1 = static_cast<int>(310*scale_factor);
const int rect_desktop_shortcut_x2 = static_cast<int>(412*scale_factor);
const int rect_desktop_shortcut_y2 = static_cast<int>(345*scale_factor);


#ifdef _WIN32
const int read_eula_x = static_cast<int>(188*scale_factor);
const int read_eula_y = static_cast<int>(430*scale_factor);
const int read_eula_width = static_cast<int>(95*scale_factor);
const int read_eula_height = static_cast<int>(24*scale_factor);
const int read_eula_font_size = static_cast<int>(21*scale_factor);
const std::wstring read_eula_font = L"Arial Black";
const std::string read_eula_font1 = "Arial Black";
#else
const int read_eula_x = static_cast<int>(188*scale_factor);
const int read_eula_y = static_cast<int>(425*scale_factor);
const int read_eula_width = static_cast<int>(95*scale_factor);
const int read_eula_height = static_cast<int>(24*scale_factor);
const int read_eula_font_size = static_cast<int>(14*scale_factor);
const std::string read_eula_font = L"Arial Black";
const std::string read_eula_font1 = "Arial Black";
#endif

#ifdef _WIN32
const int click_below_to_start_font_size = static_cast<int>(26*scale_factor);
const std::wstring click_below_to_start_font = L"Arial Black";
const std::string click_below_to_start_font1 = "Arial Black";
#else
const int click_below_to_start_font_size = static_cast<int>(20*scale_factor);
const std::string click_below_to_start_font = L"Chalkboard-Bold";
const std::string click_below_to_start_font1 = "Chalkboard-Bold";
#endif


const int click_below_to_start_x = static_cast<int>(130*scale_factor);
#ifdef _WIN32
const int click_below_to_start_y = static_cast<int>(190*scale_factor);
#else
const int click_below_to_start_y = static_cast<int>(85*scale_factor);
#endif
const int click_below_to_start_width = static_cast<int>(212*scale_factor);
const int click_below_to_start_height = static_cast<int>(28*scale_factor);


const int progress_x = static_cast<int>(178*scale_factor);
const int progress_y = static_cast<int>(240*scale_factor);
#ifdef _WIN32
const int progress_width = static_cast<int>(114*scale_factor);
#else
const int progress_width = static_cast<int>(124*scale_factor);
#endif
const int progress_height = static_cast<int>(44*scale_factor);

const int progress_font_size = static_cast<int>(17*scale_factor);
const std::wstring progress_font = L"Bahnschrift Light";
const std::string progress_font1 = "Bahnschrift Light";


const int launching_x = static_cast<int>(140*scale_factor);
const int launching_y = static_cast<int>(240*scale_factor);
const int launching_width = static_cast<int>(235*scale_factor);
const int launching_height = static_cast<int>(40*scale_factor);
#ifdef _WIN32
const int launching_font_size = static_cast<int>(24*scale_factor);
#else
const int launching_font_size = static_cast<int>(20*scale_factor);
#endif
const std::wstring launching_font = L"Arial";
const std::string launching_font1 = "Arial";


const unsigned char r0=2;
const unsigned char g0=13;
const unsigned char b0=28;

const unsigned char r1=102;
const unsigned char g1=108;
const unsigned char b1=117;

const unsigned char r2=65;
const unsigned char g2=73;
const unsigned char b2=84;

const unsigned char r3=38;
const unsigned char g3=48;
const unsigned char b3=60;

const unsigned char r4=103;
const unsigned char g4=109;
const unsigned char b4=118;


#endif // WINDOW_PARAMETERS_H
