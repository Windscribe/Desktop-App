#include <QFile>
#include "../simple_crypt.h"

#define DO_ENCRYPT
//#define DO_DECRYPT


int main(int argc, char *argv[])
{

#ifdef DO_ENCRYPT
    QFile file("C:/Work/client-desktop/gui/wsappcontrol/generate_crypted/wsappcontrol_com.crt");
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray arr = file.readAll();
        std::string key = "12m1EwO0ZVICA1yHXzZq";
        std::string data(arr.data(), arr.size());
        std::string encripted = SimpleCrypt::encrypt(data, key);

        QFile file("C:/Work/client-desktop/gui/wsappcontrol/generate_crypted/wsappcontrol_com_crypted.crt");
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(encripted.c_str(), encripted.length());
        }
    }
#endif

#ifdef DO_DECRYPT
    QFile file("C:/Work/client-desktop/gui/wsappcontrol/generate_crypted/wsappcontrol_com_crypted.crt");
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray arr = file.readAll();
        std::string key = "12m1EwO0ZVICA1yHXzZq";
        std::string data(arr.data(), arr.size());
        std::string decripted = SimpleCrypt::decrypt(data, key);

        QFile file("C:/Work/client-desktop/gui/wsappcontrol/generate_crypted/wsappcontrol_com_test.crt");
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(decripted.c_str(), decripted.length());
        }
    }
#endif

    return 0;
}
