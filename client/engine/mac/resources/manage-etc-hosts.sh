#!/bin/sh

# PATH TO HOSTS FILE
ETC_HOSTS=/etc/hosts

# Hostname to add/remove.
HOSTNAME=$2

# IP FOR HOSTNAME
IP=$3

function removewindscribehosts() {
    if [ -n "$(grep 'windscribe' /etc/hosts)" ]
    then
        echo "$HOSTNAME Found in your $ETC_HOSTS, Removing now...";
        sudo sed -i".bak" "/windscribe/d" $ETC_HOSTS
    else
        echo "$HOSTNAME was not found in your $ETC_HOSTS";
    fi
}

function addhost() {
    HOSTS_LINE="$IP\t$HOSTNAME\t#added by Windscribe, do not modify."
    echo $HOSTS_LINE
    if [ -n "$(grep $HOSTNAME /etc/hosts)" ]
        then
            echo "$HOSTNAME already exists : $(grep $HOSTNAME $ETC_HOSTS)"
        else
            echo "Adding $HOSTNAME to your $ETC_HOSTS";
            sudo -- sh -c -e "echo '$HOSTS_LINE' >> /etc/hosts";

            if [ -n "$(grep $HOSTNAME /etc/hosts)" ]
                then
                    echo "$HOSTNAME was added succesfully \n $(grep $HOSTNAME /etc/hosts)";
                else
                    echo "Failed to Add $HOSTNAME";
            fi
    fi
}


if [ "$1" = "-addhost" ] ; then
    addhost

elif [ "$1" = "-removewindscribehosts" ] ; then
    removewindscribehosts
fi