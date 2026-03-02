#include "amneziawgunblockparams.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace api_responses {

bool AmneziawgUnblockParam::isValid() const
{
    // We do not check title or countries, as a WG custom config with AmneziaWG params will not have these.
    return (jc > 0) || (jmin > 0) || (jmax > 0) || (s1 > 0) || (s2 > 0) || (s3 > 0) || (s4 > 0) ||
           (!h1.isEmpty()) || (!h2.isEmpty()) || (!h3.isEmpty()) || (!h4.isEmpty()) || (!iValues.isEmpty());
}

void AmneziawgUnblockParam::setDefault()
{
    title = "Z - API Blocked";
    countries.clear();
    jc = 0;
    jmin = 0;
    jmax = 0;
    s1 = 29;
    s2 = 87;
    s3 = 0;
    s4 = 0;
    h1 = "97329410";
    h2 = "907372093";
    h3 = "1342305062";
    h4 = "1802786097";
    iValues.clear();
    iValues.append(
        "<b 0xc6000000010843290a47ba8ba2ed000044d0e3efd9326adb60561baa3bc4b52471b2d459ddcc9a508dffddc97e4d40d811d"
        "3de7bc98cf06ea85902361ca3ae66b2a99c7de96f0ace4ba4710658aefde6dec6837bc1a48f47bbd63c6e60ff494d3e1bea5f139"
        "27922401c40b0f4570d26be6806b506a9ff5f75ca86fae5f8175d4b6bfd418df9b922cdff8e60b06decfe66f2b07da61a47b5c8b"
        "32fa999d8feac21c8878b6e15ee03b8388b2afd9ffd3b46753b0284907b10747e526eebf287ff08735929c4c5e4784a5e2ad3dd8"
        "ac8200d0e99ad1219e54060ddc72813e8a3e2291ac713c5f3251c5d748fd68782a2e8eb0c021e437a79aafb253efae3ee72e1051"
        "b647c45b676d3b9e474d4f60c7bf7d328106cb94f67eaf2c991cd7043371debbf2b4159b8f80f5da0e1b18f4da35fca0a88026b3"
        "75f1082731d1cbbe9ba3ae2bfefec250ee328ded7f8330d4cda38f74a7fe10b58ace936fc83cfcb3e1ebed520f7559549a8f2056"
        "8a248e16949611057a3dd9952bae9b7be518c2b5b2568b8582c165c73a6e8f9b042ec9702704a94dd99893421310d43ffc9caf00"
        "3ff5fc7bcd718f3fa99d663a8bbad6b595ec1d4cf3c0ed1668d0541c4e5b7e5ded40c6628eb64b29f514424d08d8522ddf7b856e"
        "9b820441907177a3dbd9b958172173be8c45c8c7b1816fe4d24927f9b12778153fc118194786c6cf49bc5cf09177f73be27917a2"
        "39592f9acd9a21150abbd1ca93b1e305dc64d9883429a032c3179e0639592c248cbacec00c90bfb5d5eaf8920bf80c47085a490e"
        "ad8d0af45f6754e8ae5692f86be02c480ca2a1e6156dccf1bcb5c911c05e3c3a946ca23461669c57d287dcfa9dd187fc6a58394f"
        "0b2878c07e1d8cb6be41725d49c118e9ddbe1ae6e5d1a04b36ad98a24f0378deea84febb60b22dc81d8377fb3f21069d83e90b9e"
        "ba2b6b0ea95acf5fd0482a00d064a9b73e0b3732fde45404e22349a514151381fc6095a8204234359c28765e360a57eb222418b1"
        "1be704651e4da1b52b135d31ba63a7f06a0f7b8b6177f9bd02fb517877a1340e59d8dbe52ea8135bc80b2aa1959539803a31720a"
        "c949c7bf0785c2e413e8b83dd4fd40d8a63fbd832ecb727d0c0df04ce10dac6a7d6d75e264aaf856e7485cc2c4e1749f169e5ad4"
        "de6f89a2333e362910dd0d962e3bf59a353df5760fd15956fe32e40f863ea020e9709aa9a9ebeffc885e64231dc6fc384bc6a9e7"
        "e5c64c0eaf39f9f14a13658883ee8dd94737cb3a8c2f7020bfacb80f0122866635873e248e22b5a5abc84d507e1720d3fb5f827d"
        "75c1d39235a361a217eb0587d1639b0b31aef1fe91958220fcf934c2517dea2f1afe51cd63ac31b5f9323a427c36a5442f8a89b7"
        "494f1592666f62be0d8cf67fdf5ef38fafc55b7b4f569a105dfa9925f0a41913c6ee13064d4b83f9ee1c3231c402d68a624e2388"
        "e357144be99197dcafb92118d9a9ec6fe832771e12448a146fb5b9620a4718070b368aab646b03cce41ec4d5d9a9c880a9cff06a"
        "ba991cc0845030abbac87c67255f0373eb38444a51d0958e57c7a33042697465c84abe6791cb8f28e484c4cd04f10791ad911b0d"
        "cc217f66cb3aa5fcdbb1e2be88139c4ac2652e469122408feba59ad04f66eb8ab8c80aaf10c2ec1f80b5be111d3ccc832df2395a"
        "947e335e7908fda5dcdaa14a61f0fa7156c94b1c96e5c191d850e341adc2e22c8f69fcfa5c3e403eadc933f18be3734bc345def4"
        "f40ea3e12>");
}

AmneziawgUnblockParams::AmneziawgUnblockParams(const std::string &json)
{
    if (json.empty()) {
        // No json indicates we were unable to retrieve the unblock parameters from the API.  Supply a default for the user to attempt connection with.
        AmneziawgUnblockParam defaultParam;
        defaultParam.setDefault();
        params_.append(defaultParam);
        return;
    }

    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    if (errCode.error != QJsonParseError::ParseError::NoError) {
        return;
    }

    auto jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        return;
    }

    auto jsonData = jsonObject["data"].toObject();

    if (!jsonData.contains("params")) {
        return;
    }

    auto jsonParamsArray = jsonData["params"].toArray();
    for (const QJsonValue &value : std::as_const(jsonParamsArray)) {
        AmneziawgUnblockParam param;
        QJsonObject obj = value.toObject();

        // Parse required fields
        if (!obj.contains("title")) {
            continue;
        }

        param.title = obj["title"].toString();

        if (obj.contains("countries")) {
            const QJsonArray jsonCountries = obj["countries"].toArray();
            for (const QJsonValue &country : jsonCountries) {
                param.countries.append(country.toString());
            }
        }

        // Documentation: https://github.com/amnezia-vpn/amneziawg-go?tab=readme-ov-file#message-paddings
        param.jc = obj.value("Jc").toInt(0);
        param.jmin = obj.value("Jmin").toInt(0);
        param.jmax = obj.value("Jmax").toInt(0);
        param.s1 = obj.value("S1").toInt(0);
        param.s2 = obj.value("S2").toInt(0);
        param.s3 = obj.value("S3").toInt(0);
        param.s4 = obj.value("S4").toInt(0);
        param.h1 = obj.value("H1").toString();
        param.h2 = obj.value("H2").toString();
        param.h3 = obj.value("H3").toString();
        param.h4 = obj.value("H4").toString();

        for (int i = 1; i <= 5; ++i) {
            QString key = QString("I%1").arg(i);
            if (obj.contains(key)) {
                QString value = obj.value(key).toString();
                if (!value.isEmpty()) {
                    param.iValues.append(value);
                }
            }
        }

        params_.append(param);
    }
}

QStringList AmneziawgUnblockParams::presets() const
{
    QStringList presets;
    for (const auto &param : std::as_const(params_)) {
        presets.append(param.title);
    }
    return presets;
}

AmneziawgUnblockParam AmneziawgUnblockParams::getUnblockParamForPreset(const QString &preset)
{
    AmneziawgUnblockParam param;

    // We'll have an empty preset if the user has not yet selected one.  E.g. anticensorship was auto-enabled.
    if (!preset.isEmpty()) {
        for (const auto &entry : std::as_const(params_)) {
            if (entry.title == preset) {
                param = entry;
                break;
            }
        }
    }

    // If we didn't locate the preset, or it is empty, select the first parameter set in the list.  If we do not have
    // any (e.g. the API is blocked or hasn't somehow completed yet) use a default set.
    if (!param.isValid()) {
        if (params_.isEmpty()) {
            param.setDefault();
        } else {
            param = params_.first();
        }
    }

    return param;
}

QDebug operator<<(QDebug debug, const AmneziawgUnblockParam &param)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "AmneziawgUnblockParam("
                    << "title=" << param.title
                    << ", countries=" << param.countries
                    << ", jc=" << param.jc
                    << ", jmin=" << param.jmin
                    << ", jmax=" << param.jmax
                    << ", s1=" << param.s1
                    << ", s2=" << param.s2
                    << ", s3=" << param.s3
                    << ", s4=" << param.s4
                    << ", h1=" << param.h1
                    << ", h2=" << param.h2
                    << ", h3=" << param.h3
                    << ", h4=" << param.h4
                    << ", iValues=" << param.iValues
                    << ")";
    return debug;
}

} // namespace api_responses
