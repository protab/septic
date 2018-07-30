# What is Septic

Septic (Server Environment for ProTab Informatics Contest) is a tool
providing run time environment for computer science contests. The attendees
solve tasks by writing a program in Python. The Python program is uploaded
to a server (the expected mode of operation is the server providing a web
based code editor) and run there in a sandboxed environment.

The task specifies what objects (functions, classes, ...) are exported to
the sandboxed program and its maximum run time.

# Building and running

## Requirements

Python 3.6 or newer.

## Building for task development

```
git submodule update --init
make
sudo make rights
```

## Running tasks

While developing a task, use the commands below to run it. Note that by
default, the tasks are looked for in `examples/tasks`.

To start the server:

```
./septic
```

To run a particular user program against the given task:


```
./client -t guess_number -p examples/solution/guess_number.py
```

Note that while the `-t` option looks for the task in a predefined location
(determined by the `tasks_dir` [config option](#configuration)) and the task
name cannot contain slashes or the extension, the `-p` option requires
a full path.

# Task definition

## Exporting

The objects, functions and classes that are to be available to the user need
to be exported. There's an `export` decorator available automatically to the
task.

The `export` decorator can be used on a function or on a class:

```python
@export
def my_func():
    ...
```

```python
@export
class MyClass:
    ...
```

To export an object, `export` can be used as a function. The first argument
is the name under which it should be exported, the second is the object:

```python
pi_digits = [ 3, 1, 4, 1, 5, 9 ]
export('puzzle', pi_digits)
```

## Exporting a class

The `export` decorator allows selection of which methods are available to
the user and which are kept private to the task. This is achieved by
applying the `export` decorator to the methods that are to be exported.

Methods that are not exported are not callable by the user. (But see the
chapter [Returning objects](#returning-objects)).

If `__init__` is not exported, the user can not instantiate the given
class. Beware that it is still possible to call the exported methods as
class methods.

`__call__` is **always exported**, no need to decorate it. It will
actually raise an exception if decorated.

If a class is not exported, it is still possible for the task to return
instances (objects) of the class. Even in such case, only the exported
methods can be called by the user on the given object.

When called by the user, the exported methods run **in the context of the
task program**, not in the user program. As such, they are allowed to call
non-exported methods, access the file system, do not have limited resources,
etc.

The user has no way to list the exported methods. Neither the `dir` command
nor printing the object's `__dict__` will reveal them.

Example:

```python
@export
class MyClass:
    def private_method():
       ...

    @export
    def exported_method():
       ...
```

## Exporting an object

Any object can be exported, including a string or an integer. Note however
that objects of the basic types (int, str, list, dict, etc.) are transferred
to the user program and processed there. If you need to alter behavior of
a basic type, simply subclass it. That way, the user program will receive
only a reference to the object and all methods will be processed in the task
program.

## Exporting under a different name

The `export` decorator accepts a keyword argument `name`. It allows to
export the given function or class under a different name. It doesn't work
for methods, though (this limitation can be lifted if there's a demand).

Example:

```python
@export(name='your_func')
def my_func():
    ...
```

## Exporting large data

When a list, tuple or dict object is exported, it is copied to the user.
However, this is not always feasible. In particular, when the data in
question are too large, it will hit the internal message size limit.

In such case, the list, tuple or dict object may be exported as an object by
passing a keyword argument `large`. Note that any access to such object will
be done as a remote call.

## Returning objects

If an object is returned by a function or method called by the user, that
object is made accessible to the user.

In particular:

* If a function is returned, it can be called by the user, even if it is not
  exported.
* If an object is returned, its exported methods (and only its exported
  methods) may be called by the user. The class of the object does not need
  to be exported.
* If a class is returned, similar rules as for the objects apply. If the
  class has its `__init__` method exported, it can be instantiated by the
  user.
* If an object of a basic type (int, str, list, dict, etc.) is returned, it
  is transfered to the user. It works recursively: complex types (lists,
  tuples, dicts) may contain references to arbitrary objects or other basic
  types and those are handled as expected.

## Output from the task

Do not use `print()` in production. Output of `print` goes to the server
stdout which is not what you want. It is useful for debugging while
developing the task, though.

To output to the user, use `uprint` instead. This function is available to
the task automatically.

## Input from the user program

A call to Python `input()` function is supported from a user program. It
is not supported from a task (this limitation can be lifted if there's a
demand).

# Integration

## Building for deployment

These examples assume that Septic will be installed into `/opt/septic` and
that the task files are in `/opt/septic/tasks`:

```
git submodule update --init
echo "install_dir = /opt/septic" > config.local
echo "tasks_dir = /opt/septic/tasks" >> config.local
make
sudo make install
sudo make mkdirs UID=septic
sudo make systemd
```

## Running user programs

The `client` tool is meant for development only. In production, a unix
socket should be used instead. The socket name is 'socket' and is located in
the `run_dir` specified in the [configuration](#configuration).

The protocol is simple: newline separated pairs of `key:value`. The
recognized keys are `login`, `task`, `prg`, `max_secs` and `action`. The
`action` is `1` for running a new program and `2` for killing the running
program. There's also a non-mandatory `token` key whose value is an
arbitrary string. The token value is not interpreted by Septic. It is
simply stored in the meta directory and can be used for any purpose by the
integrator.

As a reply, an error string is returned optionally followed by a pipe
character (`|`) and auxiliary data. If the error string is `"ok"`, the task
handling was successfuly started and the corresponding meta files were
created (note that it doesn't mean the task didn't crash immediately
afterwards).

The list of users is read from `db_path` specified in the
[configuration](#configuration). The format of the file is newline separated
list of users, each line consisting of a space separated unique user id (0
and up) and user name.

## Meta directory

When a job is submitted, a path to its meta directory is returned as
auxiliary data. Regardless of this, the current meta directory for the given
user can always be accessed under `meta_dir/username/0/`.

The meta directory contains the following files that may be of interest:

* `status`: if empty, the task is still running. Otherwise, contains the
  result of running the sandbox. See `isolate` man page for more details.
* `output`: output of the user program suitable to be served to the user.
* `input`: when present, contains a prompt message. See
  [below](#implementing-input) for input handling.
* `program.py`: a copy of the user program.
* `token`: the token if it was set. Otherwise, this file is not created.

## Implementing input

When the `input` file appears, the user program is expecting one line of
input. The `input` file contains a prompt message.

Handling of input should be done in the following steps:

1. A presence of the `input` file is detected.
2. Its content is read and displayed as a prompt.
3. User input is obtained (e.g. using a web form).
4. If the `input` file disappeared meanwhile, abort.
5. The `input` file is deleted.
6. The obtained user input is written to a temporary file.
7. The temporary file is renamed to `input.ret`.

# Advanced

## Configuration

To override the default configuration, create or edit the `config.local`
file in the source tree. Look for the values and documentation in the
`config.defaults` file.

## Septic development

For development of the Septic tool itself, follow the steps in [Building for
task development](#building-for-task-development).

Do not forget to run `make test` before submitting patches.

To submit patches, please create a pull request on
[GitHub](https://github.com/protab/septic).
