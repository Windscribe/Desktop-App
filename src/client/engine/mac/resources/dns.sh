#!/bin/bash

PATH=/usr/sbin:$PATH

# Arbitrary UUID
SERVICE_UUID="019566B0-ECE7-707F-9A54-EED796210EB0"

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

    # Save the current global settings
    echo "Saving current global network settings"
    scutil <<_EOF
        d.init
        get State:/Network/Global/IPv4
        set State:/Network/Windscribe/Original/Global/IPv4

        d.init
        get State:/Network/Global/DNS
        set State:/Network/Windscribe/Original/Global/DNS
_EOF

    # Create a new network service with the VPN DNS
    VPN_IF="$dev"
    VPN_IF_INDEX=`ifconfig -v ${VPN_IF} 2>/dev/null | grep -w index | awk '{print $6}'`
    VPN_ROUTER=`ifconfig -v ${VPN_IF} 2>/dev/null | grep -w inet | awk '{print $2}'`
    if [[ "${VPN_ROUTER}" == "" ]]; then
        VPN_ROUTER=${ifconfig_local}
    fi

    echo "Creating new network service with UUID: ${SERVICE_UUID}"
    scutil <<_EOF
        d.init
        d.add ServerAddresses * ${VPN_DNS}
        d.add DomainName "${DOMAIN_NAME}"
        d.add ConfirmedServiceID "${SERVICE_UUID}"
        d.add InterfaceName "${VPN_IF}"
        set State:/Network/Service/${SERVICE_UUID}/DNS

        # Set the PrimaryRank to First for this service
        d.init
        d.add PrimaryRank "First"
        set State:/Network/Service/${SERVICE_UUID}

        d.init
        d.add Addresses * ${VPN_ROUTER}
        d.add InterfaceName "${VPN_IF}"
        d.add Router "${VPN_ROUTER}"
        set State:/Network/Service/${SERVICE_UUID}/IPv4

        d.init
        d.add ServerAddresses * ${VPN_DNS}
        d.add DomainName "${DOMAIN_NAME}"
        d.add "__CONFIGURATION_ID__" "Default: 0"
        d.add "__FLAGS__" 2
        d.add "__IF_INDEX__" ${VPN_IF_INDEX}
        d.add "__ORDER__" 0
        set State:/Network/Global/DNS

        d.init
        d.add PrimaryService ${SERVICE_UUID}
        d.add PrimaryInterface ${VPN_IF}
        d.add Router ${VPN_ROUTER}
        set State:/Network/Global/IPv4
_EOF

elif [ "$1" = "-down" ] ; then
    echo "Restoring original global network settings"

    # Check if we have saved settings
    SAVED_IPV4=`echo "show State:/Network/Windscribe/Original/Global/IPv4" | scutil | grep PrimaryService`
    SAVED_DNS=`echo "show State:/Network/Windscribe/Original/Global/DNS" | scutil | grep ServerAddresses`

    if [[ "${SAVED_IPV4}" == "" ]]; then
        echo "Error: Cannot find original Global IPv4 configuration"
    else
        echo "Found original Global IPv4 configuration"

        # Restore original global settings
        scutil <<_EOF
            d.init
            get State:/Network/Windscribe/Original/Global/IPv4
            set State:/Network/Global/IPv4
_EOF
    fi

    if [[ "${SAVED_DNS}" == "" ]]; then
        echo "Error: Cannot find original Global DNS configuration"
    else
        echo "Found original Global DNS configuration"

        # Restore original global DNS settings
        scutil <<_EOF
            d.init
            get State:/Network/Windscribe/Original/Global/DNS
            set State:/Network/Global/DNS
_EOF
    fi

    scutil <<_EOF
        remove State:/Network/Service/${SERVICE_UUID}
        remove State:/Network/Service/${SERVICE_UUID}/DNS
        remove State:/Network/Service/${SERVICE_UUID}/IPv4
        remove State:/Network/Windscribe/Original/Global/IPv4
        remove State:/Network/Windscribe/Original/Global/DNS
_EOF

fi
