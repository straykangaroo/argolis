# Argolis v1.0

Argolis is a __no-nonsense__, small, header-only, __callback-based__ command line parsing library for C++, with minimal but effective error checking capabilities.


## Features:
    - Callback-based: you register your handlers for options, arguments and errors, and they will be invoked synchronously during the parsing phase, no need for a subsequent step asking "did you see option '-x' ?"
    - Options allowed in short (`-a`) or, optionally, long (`--all`, `-all`) form
    - Options can be specified to require / forbid / do not mind being followed by an argument
    - Option arguments may follow the option itself (`--count 42`) or be joined to it in a key-value form (`--count=42`)
    - Combi options can be allowed (e.g. `-avc <arg>` being equivalent to `-a -v -c <arg>`)
    - "End of options" marker (i.e. `--`) supported
    - Include-only library
    - Doxygen documentation
    - Requires C++17


## Getting started

### Prerequisites

C++17


### Install

Just copy ```include/argolis.hpp``` to your project's include directory


### Usage

Instantiate a ```Parser``` object, with as many ```Opt_Spec``` objects you want. In each ```Opt_Spec``` object put:
- the "short" name of the option (e.g. 'v')
- optionally, the "long" name of the option (e.g. "verbose")
- choose  what the argument policy will be for this option:
    - ```NO_ARG``` this option will not accept an argument
    - ```MAYBE_ARG``` this option might accept an argument
    - ```EXPECT_ARG``` this option requires an argument
- the *Callable* action you want to be called when this option is met on the command line

```cpp
using namespace argolis;

Parser parser({
        {'a',"all",Opt_Spec::Arg_Policy::NO_ARG,[](const Opt_Spec &,std::optional<std::string_view>){ /* YOUR CODE HANDLING OPTION "-a" */}}
        /* ... MORE OPTIONS SPECIFICATIONS */
});
```

Assign a *Callable* action you want to be called for each free-standing argument
```cpp
parser.on_arg([](std::string_view){ /* YOUR CODE */ });
```

Assign a *Callable* action you want to be called when a parsing error occurs
```cpp
parser.on_err([](std::string_view){ /* YOUR CODE */ });
```

Start parsing
```cpp
parser.parse(argc,argv);
```

### Building the API documentation (HTML format)

```sh
make doc
```


### Building the examples

```sh
make example
```


### Quick example
(see examples directory/page for more)

```cpp

#include "argolis.hpp"

int main(int argc, char * argv[]) {

    using namespace argolis;

    Parser p({
        {'a',"all",Opt_Spec::Arg_Policy::NO_ARG,[](auto,auto){ /* WE GOT OPTION "all" ! */}},
        {'v',"verbose",Opt_Spec::Arg_Policy::MAYBE_ARG,[](auto,auto){ /* WE GOT OPTION "verbose" ! */}},
        {'z',Opt_Spec::Arg_Policy::MAYBE_ARG,[](auto,auto){ /* WE GOT OPTION 'z' ! */}},
        {'n',"num",Opt_Spec::Arg_Policy::EXPECT_ARG,[](auto,auto){ /* WE GOT OPTION "num" ! */}}
    });

    p.combi_allowed(true);
    
    p.abort_on_error(true);
    
    p.on_err([](Error e){ /* WE GOT AN ERR! */ });
    
    p.on_err([](std::string_view a){ /* WE GOT A FREE-STANDING ARGUMENT! */ });
    
    p.parse(argc,argv);

    return 0;

}

```


## License

Distributed under the MIT License. See LICENSE.txt for more information.


## Contact

[https://github.com/straykangaroo/argolis](https://github.com/straykangaroo/argolis)
