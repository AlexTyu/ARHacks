r"""

An interpreter that provides blot-like syntax and functionality.  To run
the interpreter simply call the start function.

"""

#==============================================================================
#
#  Program:   ParaView
#  Module:    pvblot.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================

#-------------------------------------------------------------------------
# Copyright 2009 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
#-------------------------------------------------------------------------

import cmd
import inspect
import blotish
import blot_common
import traceback
import sys

class _PVBlotInterp(cmd.Cmd):
    """pvblot - A blot-like interpreter using ParaView as a back end.

    The pvblot interpreter reads commands in a manner similar to the SEACAS
    blot command and performs visualization using the ParaView framework.
    The images you get from pvblot will often provide more information than
    those generated by the original blot tool.  Comments begin with "$" and
    continue to the end of the line.  Commands are case insensitive and can
    be abbreviated with any unique prefix.  Keep in mind that there are
    some differences in the blot commands and those offered here.  There
    are also many blot commands that are not replicated here.
    """
    prompt = "BLOT> "
    _blotish_commands = {}
    def __init__(self, completekey='tab', stdin=None, stdout=None):
        cmd.Cmd.__init__(self, completekey, stdin, stdout)
        for funcname, func in blotish.__dict__.iteritems():
            if funcname[0] == '_' or not inspect.isfunction(func): continue
            if not func.__doc__ or not len(func.__doc__):
                raise blotish.BlotishError("Method %s is undocumented." % funcname)
            self._blotish_commands[funcname] = func
        # Insert dummy commands for help.
        self._blotish_commands['exit'] = _PVBlotInterp.do_exit
        self._error_flag = False

    def get_unique_command(self, command_name):
        """Given a command name, returns the function that implements it.
        The command name should be in all lower case.  The command name
        need only cover the first part of the command uniquely.  If no
        command is found, or more than one command is found, raises a
        BlotishError."""
        valid_commands = []
        for existing_command in self._blotish_commands.keys():
            if existing_command.startswith(command_name):
                valid_commands.append(existing_command)
        if not valid_commands:
            # If no command was found, maybe the command name is a variable name
            func = blotish._find_variable_command(command_name)
            if func is not None:
                return func
            else:
                raise blotish.BlotishError("No such command '%s'" % command_name)
        if len(valid_commands) > 1:
            raise blotish.BlotishError(
                  "Command not found.  Possible commands: %s" % str(", ").join(valid_commands))
        return self._blotish_commands[valid_commands[0]]

    def remove_comments_from_line(self, line):
        """Searches a command string for the comment character.  If found,
        removes the comment character and everything following it to create
        a new command string.  Returns the new command string converted to
        lowercase."""
        comment_start = line.find('$')
        if (comment_start >= 0):
            line = line[:comment_start]
        return line.lower()

    def precmd(self, command):
        """This method is called to preprocess a command input."""
        return self.remove_comments_from_line(command)

    def do_help(self, command_name):
        """The given argument is a string representing a command name.  The
        string may be empty.  Prints documentation for the command name if given
        else prints a list of available command names."""
        if not command_name:
            print _PVBlotInterp.__doc__
            print "The following commands are supported:"
            print "  ",
            blotish_commands = self._blotish_commands.keys()
            blotish_commands.sort()
            for c in blotish_commands:
                print c,
            print
            print
            print "For more information on any command, try help <command>."
            return
        try:
            command = self.get_unique_command(command_name)
            print command.__doc__
        except blotish.BlotishError, err:
            blot_common.print_blot_error(err)

    def do_exit(self, args):
        "Exit the blot interpreter."
        return True

    def do_eof(self, line):
        """Print a blank line and return True to signal we are ready to exit."""
        print ""
        return True

    def handle_exception(self, exception):
        """Print a formatted error message if the exception is a BlotishError
        else print a traceback.  Does not cause the interpreter to quit."""
        self._error_flag = True
        if exception.__class__ is blotish.BlotishError:
            blot_common.print_blot_error(exception)
        else:
            traceback.print_exc(file=sys.stdout)

    def handle_async_input(self, s):
        """Takes an input string.  If there is an object waiting for asynchronous
        input that uses the string then return true, if an exception is thrown then
        it prints the exception and possibly a traceback and returns true.  If the
        input is not used at all then return false."""
        ret = True
        try: ret = blotish._handle_input(s)
        except Exception, err:
            self.handle_exception(err)

        # Update the prompt, some commands cause the prompt to change
        self.prompt = blotish._get_prompt()
        return ret
      
    def emptyline(self):
        """If an object is waiting for asynchronous input pass it the empty string"""
        self.handle_async_input("")

    def default(self, s):
        """This method is called to handle all blotish commands."""

        # First see if the asynchronous IO helper eats the string
        if self.handle_async_input(s):
            return False

        # Split the string into tokens.  Treat the first token as the command
        # name, look for a blotish command and call it with the remaining tokens.
        args = s.split()
        try:
            command = self.get_unique_command(args[0])
            command(*args[1:])
        except Exception, err:
            self.handle_exception(err)

        # Update the prompt, some commands cause the prompt to change
        self.prompt = blotish._get_prompt()



def initialize(filename=None):
    """Initialize the pvblot interpreter for non-interactive use.

    Normally you just call start and it takes care of reading and executing
    commands.  However, in situation where you are not reading from stdin and
    stdout, you may need some other controlling mechanism to call methods
    one at a time.  In that case, use the initialize, execute, and finalize
    functions.
    """
    # Turn off progress printing while running
    import paraview.servermanager
    global wasProgressPrinting
    wasProgressPrinting = paraview.servermanager.GetProgressPrintingIsEnabled()
    paraview.servermanager.SetProgressPrintingEnabled(False)

    blotish._init_blotish(filename)
    global interpreter
    interpreter = _PVBlotInterp()

def execute(command):
    """Run a command in the interpreter.

    Normally you just call start and it takes care of reading and executing
    commands.  However, in situation where you are not reading from stdin and
    stdout, you may need some other controlling mechanism to call methods
    one at a time.  In that case, use the initialize, execute, and finalize
    functions.

    This function returns a true value if the exit command is given.
    """
    global interpreter
    command = interpreter.precmd(command)
    stop = interpreter.onecmd(command)
    stop = interpreter.postcmd(stop, command)
    return stop

def finalize():
    """Clean up the interpreter.

    Normally you just call start and it takes care of reading and executing
    commands.  However, in situation where you are not reading from stdin and
    stdout, you may need some other controlling mechanism to call methods
    one at a time.  In that case, use the initialize, execute, and finalize
    functions.
    """
    global interpreter
    del interpreter
    blotish._cleanup()

    # Set the progress printing state to whatever it was before
    import paraview.servermanager
    global wasProgressPrinting
    paraview.servermanager.SetProgressPrintingEnabled(wasProgressPrinting)

def execute_file(filename):
    """Executes each command in the file.  If an exit command is executed
    this method will stop execution and return True.  This method returns
    False after all commands have been executed.  If a command causes an
    error then execution stops but this method will still return False.
    This can only be called after calling initialize()"""
    try:
        f = open(filename, 'r')
    except IOError, err:
        print "Could not open file", filename
        return
    blotish._set_interactive(False)
    exit_flag = False
    for line in f:
        line = line.rstrip()
        exit_flag = execute(line)
        if exit_flag: break
        if interpreter._error_flag: break
    blotish._set_interactive(True)
    return exit_flag
        


def start(data_file, script_file=None):
    """Start the pvblot interpreter."""

    # Try to start up the interpreter
    try:
        initialize(data_file)
    except blotish.BlotishError, err:
        blot_common.print_blot_error(err)
        return

    # Maybe run a script
    exit_flag = False
    if script_file:
        exit_flag = execute_file(script_file)

    # Start the interpreter unless the script called exit
    if not exit_flag:
        global interpreter
        interpreter.cmdloop()

    # Cleanup
    finalize()


def main():
    """
    Call start() after parsing sys.argv for arguments:
        [-input script_file] data_file
    """

    data_file = None
    script_file = None
    if sys.argv and len(sys.argv) > 1:
        if sys.argv[1] == "-input":
            try: script_file = sys.argv[2]
            except IndexError:
                print "You need to specify a blot script for the -input option."
                return
            try: data_file = sys.argv[3]
            except IndexError:
                print "You need to specify a data filename."
                return
        else: data_file = sys.argv[1]
    start(data_file, script_file)


#If we are running this source file directly, start it up.
if __name__ == '__main__':
    main()

