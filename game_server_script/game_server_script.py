#!/usr/bin/env python2
import pickle
import random
import socket
import string
import sys
import traceback
import os

PORT = 1234
STORAGE = "addoctive_storage/"

# for each connection! Keep in mind that the gameserver script
# connects up to four times each round.
TIMEOUT = 5

# return values
SUCCESS = 0
GENERIC = 1
FLAGERR = 5
GARBLED = 9
NETWORK = 13
FOULERR = 21

class Connection(object):
    def __init__(self, target, timeout):
        self.sock = socket.create_connection(target, timeout)
        self.sock.settimeout(timeout)

    def read_until(self, f):
        if not callable(f):
            f = lambda s, substr=f: substr in s
        buf = ""
        while not f(buf):
            d = self.sock.recv(1)
            assert d
            buf += d
        return buf

    def read_line(self):
        return self.read_until("\n")[:-1]

    def write(self, s):
        self.sock.send(str(s))

    def write_line(self, line):
        self.write(str(line) + "\n")

    def read_all(self):
        buf = ""
        while True:
            d = self.sock.recv(1)
            if not d:
                return buf
            buf += d

class Addoctive(Connection):
    def __init__(self, target, timeout):
        super(Addoctive,self).__init__(target, timeout)

    def wait_for_command_prompt(self):
        self.read_until("Type the command number: ")

    def exit(self):
        self.wait_for_command_prompt()
        self.write_line(0)
        assert "Thank you for choosing" in self.read_all()

    def register(self, username, pw):
        self.wait_for_command_prompt()
        self.write_line(1)
        self.write_line(username)
        self.write_line(pw)
        yes = "successfully"
        no = "already exists"
        answer = self.read_until(lambda s: yes in s or no in s)
        assert yes in answer

    def login(self, username, pw):
        self.wait_for_command_prompt()
        self.write_line(2)
        self.write_line(username)
        self.write_line(pw)
        yes = "how are you today"
        no = "failed"
        answer = self.read_until(lambda s: yes in s or no in s)
        assert yes in answer

    def list_users(self):
        self.wait_for_command_prompt()
        self.write_line(3)
        self.read_until("Available users:\n")
        res = self.read_until("------------\n")
        return res.split("\n")[:-2]

    def logout(self):
        self.wait_for_command_prompt()
        self.write_line(0)
        self.read_until("Bye bye")

    def show_user_info(self):
        self.wait_for_command_prompt()
        self.write_line(1)
        self.read_until("Username: ")
        user = self.read_line()
        self.read_until("Password: ")
        pw = self.read_line()
        return (user, pw)

    def list_templates(self):
        self.wait_for_command_prompt()
        self.write_line(2)
        self.read_until("Available templates:\n")
        res = self.read_until("------------\n")
        return res.split("\n")[:-2]

    def read_template(self, name):
        self.wait_for_command_prompt()
        self.write_line(3)
        self.read_until("want to show? ")
        self.write_line(name)
        yes = "-----------\n"
        no = "Template not found\n"
        answer = self.read_until(lambda s: yes in s or no in s)
        assert yes in answer
        return "\n".join(answer.split("\n")[1:-2])

    def create_template(self, name, content):
        self.wait_for_command_prompt()
        self.write_line(4)
        self.read_until("want to create? ")
        self.write_line(name)
        yes = "EOF:\n"
        no = "Could not open"
        answer = self.read_until(lambda s: yes in s or no in s)
        assert yes in answer
        self.write(content + "\nEOF\n")
        self.read_until("---------\n")

    def instantiate_template(self, name, var1, var2, var3):
        self.wait_for_command_prompt()
        self.write_line(5)
        self.read_until("want to use? ")
        self.write_line(name)
        self.write_line(var1)
        self.write_line(var2)
        self.write_line(var3)
        self.read_until("copy & paste:\n")
        answer = self.read_until("------------\n")
        return "\n".join(answer.split("\n")[:-2])

##
# Print a message on stderr
##
def error(msg):
    sys.stderr.write(str(msg) + "\n")

def print_tb():
    traceback.print_exc(file=sys.stderr)

rand = random.SystemRandom()
words = open('english_words.txt').read().split()

# valid alphabet, should not break the app
alph = "".join(chr(c) for c in range(ord(" "), ord("~")+1) if chr(c) not in "'.%/")

##
# Get random string of n characters
##
def randomString(n, alph=alph):
    return ''.join(rand.choice(alph) for _ in range(n))

def randomName(n):
    return randomString(n, string.ascii_uppercase + string.digits)

def randomWord():
    return rand.choice(words)

def randomToken():
    w = randomWord()
    x = rand.randint(1,10)
    if x <= 7:
        return w
    if x <= 9:
        return w[0].upper() + w[1:]
    return w.upper()

def randomWhitespace():
    ws = ""
    for _ in range(rand.randint(1,4)):
        ws += random.choice("       \n")
    return ws

def randomText(lo=5, hi=100):
    text = ""
    if rand.randint(1,2) == 1:
        text += randomWhitespace()
    for i in range(rand.randint(lo, hi)):
        if i > 0:
            text += randomWhitespace()
        if random.randint(1,10) == 1:
            text += random.choice([chr(c) for c in range(0,255) if chr(c) != '%'])
        text += randomToken()
    if rand.randint(1,2) == 1:
        text += randomWhitespace()
    return text

def insertRandom(text, insert):
    i = random.randint(0, len(text))
    return text[:i] + insert + text[i:]

def connect(ip):
    target = (ip, PORT)
    return Addoctive(target, TIMEOUT)

##
# Store a flag
##
def store(ip, id, flag):
    try:
        username = randomName(rand.randint(6, 10))
        password = randomString(50)
        templates = []

        template_content = randomText()
        if random.randint(1,2) == 1:
            template_content = insertRandom(template_content, "[1!]")
            if random.randint(1,2) == 1:
                template_content = insertRandom(template_content, "[2!]")
        template_content = insertRandom(template_content, "flag{%s}" % flag)
        #print template_content

        a = connect(ip)
        a.register(username, password)
        a.login(username, password)

        num_templates = random.randint(1,5)
        main_template = random.randrange(0,num_templates)
        # create templates
        for i in range(num_templates):
            name = randomName(rand.randint(1,100))
            templates.append(name)
            content = randomText()
            if random.randint(1,2) == 1:
                content = insertRandom(content, "[1!]")
                if random.randint(1,2) == 1:
                    content = insertRandom(content, "[2!]")
            if i == main_template:
                template_name = name
                content = insertRandom(content, "flag{%s}" % flag)
            a.create_template(name, content)

        pickle.dump((username, password, template_name, templates),
                open(STORAGE + id + ".p", "wb"))
    except:
        print_tb()
        return GARBLED
    print("SUCCESS")
    return SUCCESS

##
# Retrieve a flag
##
def retrieve(ip, id, flag):
    username, password, template_name, templates = \
            pickle.load(open(STORAGE + id + ".p", "rb"))
    try:
        a = connect(ip)
        assert username in a.list_users()
        a.login(username, password)

        u,p = a.show_user_info()
        if u != username or p != password:
            error("Show user info broken")
            return GARBLED

        # test template instantiation
        name = randomName(rand.randint(5,10))
        content = randomText()
        content = insertRandom(content, "[1!]")
        repl = randomString(rand.randint(5, 100))
        expect = content.replace("[1!]", repl)
        a.create_template(name, content)
        have = a.instantiate_template(name, repl, repl, repl)
        if have != expect:
            pickle.dump({
                'have': have,
                'expect': expect,
                'name': name,
                'content': content,
                'repl': repl,
            }, open(STORAGE + id + ".bugdump.p", "wb"))
            error("Template instantiation does not seem to work anymore")
            return GARBLED

        assert template_name in a.list_templates()
        content = a.read_template(template_name)
        if not "flag{" in content:
            error("flag marker not found")
            return GARBLED
        remote_flag = content.split("flag{")[1].split("}")[0]
        if remote_flag != flag:
            error("Invalid flag: " + repr(remote_flag) + " != " + repr(flag))
            return FLAGERR
    except:
        print_tb()
        return GARBLED
    print("SUCCESS")
    return SUCCESS


##
# Main
##
def main():
    if not os.path.exists(STORAGE):
            os.makedirs(STORAGE)
    if len(sys.argv) < 5:
        error("Usage: " + sys.argv[0] + " $action $ip $id $flag")
        print("Internal gameserver script error")
        return GENERIC

    action = sys.argv[1]
    ip = sys.argv[2]
    id = sys.argv[3]
    flag = sys.argv[4]

    if (action == "store"):
        return store(ip, id, flag)
    elif (action == "retrieve"):
        return retrieve(ip, id, flag)
    else:
        error("$action has to be 'store' or 'retrieve'")
        print("Internal gameserver script error")
        return GENERIC

sys.exit(main())
