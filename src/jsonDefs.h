String indiJSON = R"json({
    "Connection": {
        "SwitchVector": [
            {
                "name": "CONNECTION",
                "label": "Connection",
                "Switches": [
                    {
                        "name": "CONNECT",
                        "label": "Connect",
                        "value": "On"
                    },
                    {
                        "name": "DISCONNECT",
                        "label": "Disconnect",
                        "value": "Off"
                    }
                ]
            },
            {
                "name": "ON_COORD_SET",
                "label": "On Set",
                "Switches": [
                    {
                        "name": "TRACK",
                        "label": "Track",
                        "value": "On"
                    },
                    {
                        "name": "SLEW",
                        "label": "Slew",
                        "value": "Off"
                    },
                    {
                        "name": "SYNC",
                        "label": "Sync",
                        "value": "Off"
                    }
                ]
            }
        ],
        "NumberVector": [],
        "TextVector": []
    },
    "Telescope": {
        "SwitchVector": [],
        "NumberVector": [
            {
                "name": "EQUATORIAL_EOD_COORD",
                "label": "Telescope Coordinates",
                "numbers": [
                    {
                        "name": "RA",
                        "label": "RA",
                        "format": "%010.6m",
                        "value": "0"
                    },
                    {
                        "name": "DEC",
                        "label": "DEC",
                        "format": "%010.6m",
                        "value": "0"
                    }
                ]
            },
            {
                "name": "HORIZONTAL_COORD",
                "label": "Alt/Az Telescope Coordinates",
                "numbers": [
                    {
                        "name": "ALT",
                        "label": "Alt",
                        "format": "%010.6m",
                        "value": "0"
                    },
                    {
                        "name": "AZ",
                        "label": "Az",
                        "format": "%010.6m",
                        "value": "0"
                    }
                ]
            },
            {
                "name": "TARGET_EOD_COORD",
                "label": "Target Coordinates",
                "numbers": [
                    {
                        "name": "RA",
                        "label": "RA",
                        "format": "%010.6m",
                        "value": "0"
                    },
                    {
                        "name": "DEC",
                        "label": "DEC",
                        "format": "%010.6m",
                        "value": "0"
                    }
                ]
            }
        ],
        "TextVector": []
    },
    "Site": {
        "SwitchVector": [],
        "NumberVector": [
            {
                "name": "GEOGRAPHIC_COORD",
                "label": "Telescope Location",
                "numbers": [
                    {
                        "name": "LAT",
                        "label": "Lat",
                        "format": "%010.6m",
                        "value": "34.73166666"
                    },
                    {
                        "name": "LONG",
                        "label": "Lon",
                        "format": "%010.6m",
                        "value": "273.413333333"
                    },
                    {
                        "name": "ELEV",
                        "label": "Elev",
                        "format": "%010.6m",
                        "value": "193.52"
                    }
                ]
            }
        ],
        "TextVector": [
            {
                "name": "TIME_UTC",
                "label": "Time (UTC)",
                "texts": [
                    {
                        "name": "UTC",
                        "label": "UTC",
                        "value": "2026-09-23T16:38:00"
                    },
                                        {
                        "name": "OFFSET",
                        "label": "UTC Offset",
                        "value": "0"
                    }
                ]
            }
        ]
    },
    "Driver": {
        "SwitchVector": [],
        "NumberVector": [
            {
                "name": "MOTOR",
                "label": "Steps per Revolution",
                "numbers": [
                    {
                        "name": "AZ_Steps",
                        "label": "Az Steps",
                        "format": "%f",
                        "value": "20000"
                    },
                    {
                        "name": "Alt_Steps",
                        "label": "Alt Steps",
                        "format": "%f",
                        "value": "20000"
                    }
                ]
            }
        ],
        "TextVector": [
            {
                "name": "DRIVER_INFO",
                "label": "DriverInfo",
                "texts": [
                    {
                        "name": "DRIVER_NAME",
                        "label": "Name",
                        "value": "ESP32INDI"
                    },
                    {
                        "name": "DRIVER_EXEC",
                        "label": "Exec",
                        "value": "ESP32 INDI Telescope Simulator"
                    },
                    {
                        "name": "DRIVER_VERSION",
                        "label": "Version",
                        "value": "1.0"
                    },
                    {
                        "name": "DRIVER_INTERFACE",
                        "label": "Interface",
                        "value": "5"
                    }
                ]
            }
        ]
    }
})json";