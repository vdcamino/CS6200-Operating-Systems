# Installation

## Virtual Environment

You are free to use any environment you wish for your development, but your code will be evaluated using a Docker container version of Ubuntu 18.04; all feedback and grading is done based upon that environment.

1. One way to generate such an environment is to use Vagrant.  In that case, download the latest [Vagrant binary](https://www.vagrantup.com/downloads.html) for your native platform.  This should also automatically download Virtualbox, which we will use as our provider.  For more details, please see the guides below (HT: Isabel Lupiani and Charles McGuinness from Computational Photography).

    * [Windows](https://docs.google.com/document/d/1FxuHsekpU5ng1ZxULZjpHg6u145fz8cie6kgH86y2uI/edit?usp=sharing)
    * [Mac OSX](https://docs.google.com/document/d/13IZnOzZtC5ZVmSy162fkpb44M3xsbhlIzjTAbfqDjjo/edit)

2. Add the **Vagrantfile** from this repository to the top-level directory that you plan to use for the course.  For example:

    * CS6200/
        * Vagrantfile
        * pr1/
        * pr2/
        * pr3/
        * pr4/

You can copy-paste from your web-browser, or if you have already cloned the repo, run `mv pr1/Vagrantfile ./` From any subdirectory, you can start your virtual machine with just the command `vagrant up` and connect to it with just `vagrant ssh`.  Note that the vagrant machine will share the folder containing the Vagrantfile at `/vagrant`. Therefore, your first command after logging into the vagrant machine will almost always be `cd /vagrant`.  See the [Vagrant documentation](https://www.vagrantup.com/docs/getting-started/) for more details.

## Git

This and all assignments will be made available on the [https://github.gatech.edu](https://github.gatech.edu) server.  To access the code and import any updates, you will want to [install git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git).  Git on Windows is a little different, so you may want to consult this [guide for Windows](https://docs.google.com/document/d/1_geDGrI0JlHHtnJY0P5pe6yU3E19NLEejdAOyDloqdQ/edit?usp=sharing) (HT: Isabel Lupiani).

Change to the directory you plan to use for the course.  Then clone this repository with the command

```console
$ git clone https://github.gatech.edu/gios-spr-19/pr1.git
...
```

(If this command fails with a complaint about "server certificate verification failed", then you might need to run

```console
$ GIT_SSL_NO_VERIFY=true git clone --recursive https://github.gatech.edu/gios-spr-19/pr1.git
...
$ cd pr1
$ git config http.sslVerify false
```

instead. Note we don't really recommend using git without SSL, but some people have had issues with misconfiguration in the past and this is a work around, albeit one that 
pretends security is not an issue.)

**Note: the repositories won't be available until the project is released.  The first project is normally released shortly after the registration period ends.**

## Submitting Code Outside the VM

Some students prefer to develop mostly on their native environment and only use the Vagrant VM for testing.  In this setup, it is also convenient to be able to submit code from the native environment.  To do this you will need to install

* [python 2.7](https://www.python.org/downloads/)
* [python's requests library](http://docs.python-requests.org/en/master/user/install/)
* [python's future library](http://python-future.org/quickstart.html)

** Note that these steps are optional.  The VM has everything you need already **

All are very standard tools and should be easy to install by following the directions at the links above.  There is also a [guide for Windows](https://docs.google.com/document/d/1nvHUqwo2wXR6CAJYzDkVxwWxdvT8I2fodBZrn88d6E0/edit#) (HT: Isabel Lupiani).  If you have any problems, please ask for assistance on Piazza.

The scripts should be Python 3 compatible as well.
