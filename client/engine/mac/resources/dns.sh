#!/bin/bash

PATH=/usr/sbin:$PATH

CUR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
PRI_IFACE=`echo 'show State:/Network/Global/IPv4' | scutil | grep PrimaryInterface | sed -e 's/.*PrimaryInterface : //'`
PSID=`echo 'show State:/Network/Global/IPv4' | scutil | grep PrimaryService | sed -e 's/.*PrimaryService : //'`
PSID_SAVED=`echo "show State:/Network/Windscribe/DNS" | scutil | grep NetworkInterfaceID | sed -e 's/.*NetworkInterfaceID : //'`

if [[ "${PRI_IFACE}" == "" ]]; then
    echo "Error: Primary interface not found"
    echo 
    if [ "$1" != "-down" ] ; then
       exit 1
    fi
else
    echo "Primary interface: ${PRI_IFACE}"
    echo "PSID: ${PSID}"
    echo "PSID_SAVED: ${PSID_SAVED}"
    echo    
fi

function print_state {

    S_STATE=`echo "show State:/Network/Service/${PSID}/DNS" | scutil`
    S_SETUP=`echo "show Setup:/Network/Service/${PSID}/DNS" | scutil`

    echo "State: ${S_STATE}"
    echo 
    echo "Setup: ${S_SETUP}"
    echo
}

function is_dns_changed {    

    PREFIX=$1

    DNS_STATE=`echo "show ${PREFIX}:/Network/Service/${PSID}/DNS" | scutil`
    VPN_STATE=`echo "show State:/Network/Windscribe/DNS" | scutil`

    if [[ "${DNS_STATE}" == "${VPN_STATE}" ]]; then
        return 1
    fi

    return 0
}

function is_vpn_dns_state_set {
    echo "show State:/Network/Windscribe/Original/DNS/State" | scutil | grep ServerAddresses >/dev/null
}

function is_vpn_dns_setup_set {
    echo "show State:/Network/Windscribe/Original/DNS/Setup" | scutil | grep ServerAddresses >/dev/null
}


function is_dns_set_by_windscribe {
    PREFIX=$1    
    echo "show ${PREFIX}:/Network/Service/${PSID}/DNS" | scutil | grep SetByWindscribe >/dev/null
    return $?
}

function store_user_setting {
    PREFIX=$1

    echo "Storing ${PREFIX}:/ dns settings"

    if is_dns_set_by_windscribe "${PREFIX}"; then
        echo "Ignoring: Current DNS change was made by Windscribe"
        return 1
    fi

    scutil <<_EOF
    d.init
    get ${PREFIX}:/Network/Service/${PSID}/DNS
    set State:/Network/Windscribe/Original/DNS/${PREFIX}
_EOF
}

function restore_user_setting {
    PREFIX=$1

    echo "Restoring ${PREFIX}:/ dns settings"

    scutil <<_EOF
    d.init
    get State:/Network/Windscribe/Original/DNS/${PREFIX}
    set ${PREFIX}:/Network/Service/${PSID_SAVED}/DNS
_EOF
}

function update_setting {
    PREFIX=$1

    scutil <<_EOF
    d.init
    get State:/Network/Windscribe/DNS
    set ${PREFIX}:/Network/Service/${PSID}/DNS
_EOF
}

function store_and_update {
    PREFIX=$1

    if [[ "${PREFIX}" == "" ]]; then
        return 127
    fi

    store_user_setting "${PREFIX}"
    update_setting "${PREFIX}"
}

if [ "$1" = "-up" ] ; then

    DOMAIN_NAME="windscribe-client"
    FOREIGN_OPTIONS=`env | grep -E '^foreign_option_' | sort | sed -e 's/foreign_option_.*=//'`

    while read -r option
    do
        case ${option} in
            *DOMAIN*)
                DOMAIN_NAME=${option//dhcp-option DOMAIN /}
                ;;
            *DNS*)
                VPN_DNS=${option//dhcp-option DNS /}
                ;;
        esac
    done <<< "${FOREIGN_OPTIONS}"

    echo "DOMAIN: $DOMAIN_NAME"
    echo "VPN DNS: $VPN_DNS"    

    scutil <<_EOF
        d.init
        d.add ServerAddresses * ${VPN_DNS}
        d.add DomainName "${DOMAIN_NAME}"
        d.add NetworkInterfaceID "${PSID}"
        d.add SetByWindscribe "true"

        set State:/Network/Windscribe/DNS
_EOF

    store_and_update "Setup"
    store_and_update "State"

elif [ "$1" = "-down" ] ; then

    if ! is_vpn_dns_state_set ; then
        if ! is_vpn_dns_setup_set ; then
            echo "Error: Cannot find original DNS Setup and State configuration"
            exit 0
        else
            echo "Finded original DNS Setup configuration"
        fi            
    else
            echo "Finded original DNS State configuration"
    fi

    restore_user_setting "State"

    restore_user_setting "Setup"

    echo "Remove saved keys"

    scutil <<_EOF
        remove State:/Network/Windscribe/Original/DNS/Setup
        remove State:/Network/Windscribe/Original/DNS/State
        remove State:/Network/Windscribe/DNS

        quit
_EOF
else
    print_state
fi

