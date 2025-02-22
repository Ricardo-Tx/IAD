# this program is meant to test the commands implementation on `analog_serial_pi`,
# and show a limited example of serial communication from python

import serial
import time


# find the arduino port (COM# on windows, /dev/tty# on linux)
port = serial.Serial(port="COM8", baudrate=38400,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
)
time.sleep(3)

# syntactically valid commands
valid_commands = [
    "turnOn()",
    "setSpeed(100)",
    "move()",
    "move(10,20)",
    "rotate(90,50)",
    "ledControl(3,HIGH)",
    "setState(OFF)",
    "enableFeature(ON)",
    "toggle(OFF)",
    "config(1,LOW,3)",
    "setTemp(235)",
    "update(42,314,10)",
    "selfDestruct(5)",  # Now valid
    "undefinedCmd(100)",  # Now valid
    "commandName123(1,2,3)",  # 15-char command, valid
    "maxLen16Command()",  # Exactly 15-char command, valid
    "validCmd(arg1,arg2,arg3)",  # Arguments within 16 chars
    "checkNumbers(123456789012345)",  # 15-char argument allowed
    "mixedArgs(ON,12345,LOW)",  # Mixed types within limits
    "mixedArgs(1234567890,1234567890,1234567890)",  # all arguments can be loooong
]

# syntactically invalid commands
invalid_commands = [
    # 🚫 Exceeds 16-char command length
    "thisCommandIsTooLong()",  # 21 characters
    "reallyLongCommandName(1,2,3)",  # 23 characters

    # 🚫 More than 3 arguments
    "validCmd(1,2,3,4)",  # Too many arguments

    # 🚫 Exceeds 16-char argument length
    "setSpeed(12345678901234561)",  # 17 characters in argument
    "longArgs(a1234567890123451)",  # Argument too long

    # 🚫 Missing Parentheses
    "setSpeed 100",  # No parentheses
    "move 10,20",  # No parentheses around arguments
    "turnOn",  # No parentheses at all

    # 🚫 Mismatched or Missing Parentheses
    "setSpeed(",  # Opening ( but no closing )
    "turnOn))",  # Extra )
    "rotate(90,50",  # Missing closing )
    "move())",  # Extra )

    # 🚫 Using Disallowed Characters
    "move(10;20)",  # Semicolon ; instead of comma ,
    "setSpeed(100 200)",  # Space inside parentheses
    "setMode(\"A\")",  # Quotes are not allowed
    "toggle('ON')",  # Quotes are not allowed
    "setValue(10, 20)",  # Spaces are not allowed
    "playTone(440,500,300 )",  # Space at the end is not allowed

    # 🚫 Spaces in Command Name
    "move to(100,200)",  # Space in command name
    "set Speed(50)",  # Space in command name
    "turn-On()",  # Hyphen - is not allowed in command name

    # 🚫 Invalid Argument Separators
    "move(10;20)",  # Semicolon ; instead of comma ,
    "rotate(90,,50)",  # Double commas are not allowed
    "setSpeed(100,200,)",  # Trailing comma is not allowed

    # 🚫 Incorrect Number of Arguments
    "rotate(90,,50)",  # Double commas are invalid
    "playTone(440,500,300,600)",  # Too many arguments

    # 🚫 Invalid Characters
    "command@name(10,20,30)",  # @ not allowed
    "invalid#cmd(100)",  # # not allowed
    "weird$name(50)",  # $ not allowed

    "name(50)aaa",  # text after is not allowed
    "undefinedCommand(100)",    # command too long
    "undefinedCommands(100)",    # command too long
    "checkNumbers(1234567890123456)",  # 16-char argument not allowed
]


print("VALID COMMANDS")
for cmd in valid_commands:
    print(f"Test: {cmd:<64}", end="")
    port.write(bytes(cmd+"\n", encoding="ascii"))
    time.sleep(0.1)

    last_line = port.read_all().decode("ascii").strip("\r\n").split("\n")[-1]
    print(last_line)
    #time.sleep(0.5)

print("\nINVALID COMMANDS")
for cmd in invalid_commands:
    print(f"Test: {cmd:<64}", end="")
    port.write(bytes(cmd+"\n", encoding="ascii"))
    time.sleep(0.1)

    last_line = port.read_all().decode("ascii").strip("\r\n").split("\n")[-1]
    print(last_line)
    #time.sleep(0.5)
