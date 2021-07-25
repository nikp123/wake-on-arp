# wake-on-arp
[![Donation](https://img.shields.io/liberapay/patrons/nikp123.svg?logo=liberapay)](https://liberapay.com/nikp123/donate)

An commandline daemon that wakes up a device on the local network when accessed

# What does this do, exactly?
You could think of this program as an "automatic Wake-On-LAN" daemon.

Let's suppose you have a large server, media box or even your PC
and you want to easily access those devices from a network while saving power.

Instead of manually sending WOL "magic packets" using a program or app,
what if instead we could wake up the device when accessed. Example being,
ssh-ing into a Mac Mini while in sleep mode.

# How does it do this?

Well, quite simply actually. It detects outgoing ARP requests (basically asking
the router whether or not we can get to the device) and if they match the host
and target it sends a WOL packet.

## Don't modern network cards feature "ARP mode" by default?

Yes, but not all of them. Certain Broadcom ethernet chips do not feature waking on ARP,
which forced me to make this program by myself.

### WAIT, THERE's A ARP MODE?

``sudo ethtool -s yournetworkdevice0 wol a``

You'll get a ``Operation not supported`` otherwise if your card doesn't support it.

## You are lazy

Most programmers are lazy bastards.

# What is the use-case of this?

Use-case 1: Large server that drains Watts upon Watts of power and your landowner is
 yelling at you because of the high-power bills. But you have a small and power-efficient
 secondary device, such as a Raspberry Pi that could indeed run 24/7 without using much power. 
 Using the Raspberry Pi, you could route all the big server traffic to the Raspberry Pi and 
 using simple proxies reroute it back to the big server. Nginx supports this out of the box.
 This assumes you know how to automatically suspend your server, which I won't get into.

Use-case 2: You are too lazy to wake up your NAS at home using a 3rd party program, so instead you
 can run this as a daemon in the background on any UNIX-like OS. Don't worry, this program uses only
 a few kilobytes of RAM and barely any CPU time since it's all UNIX code without any dependencies.

# What do I need to run this?
 * A functioning and complete computer that runs a UNIX-like OS
 * A network card (Wi-Fi works too) and connection too
 * A device that supports WOL
 * Any functional C compiler in existence and make

# How to compile

``make``

# How to run

``./wake-on-arp -h`` to see what arguments you have to fill in

## I don't want to fill in commandline arguments in my init script/daemon!

There's a config file for that, it's (usually) located in ``/etc/wake-on-arp.conf``

However, it's only there when you run ``make install``

# How to install

Once compiled,
``make install`` (as root)

## Systemd.service example

If you like, you can use systemd to run and monitor this tool

`sudo vi /lib/systemd/system/wakeonarp.service`

```INI
[Unit]
Description=Wake on LAN based on ARP
After=network-online.target network.target rsyslog.service
Wants=network-online.target network.target rsyslog.service

[Service]
Type=simple
ExecStart=/usr/local/bin/wake-on-arp
Restart=on-failure
#in case network was not online
RestartSec=15

[Install]
WantedBy=multi-user.target
```

`sudo systemctl enable wakeonarp.service`

`sudo systemctl start wakeonarp.service`

## Configure persistent logging for analytics

If you use a subnet mask fitting the whole network, you may want to monitor which IP did wake up your target.

If the event was in the last days or since last reboot, you don't any additional configuration and can just use

`systemctl status wakeonarp.service` or `journalctl -u wakeonarp.service`

But if you need to archiv the wakeup reasons, you may want to store it outside of the journal files with the help of rsyslog.

`sudo vi /etc/rsyslog.d/wake-on-arp.conf`

```
if $programname == 'wake-on-arp' then /var/log/wake-on-arp.log
```

`sudo systemctl restart rsyslog`


# LICENSE
 It's included in this repository. However, since the repository features code from other projects,
 it also includes licenses from these repositories (for those specific parts):
 * https://github.com/meetrp/sniffer.c
 * https://github.com/GramThanos/WakeOnLAN
 * This I guess: https://stackoverflow.com/a/2283541/3832385

