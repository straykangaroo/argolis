#include "argolis.hpp"

#include <cassert>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <string>

void usage()
{
    std::cout << "usage:\n";
    std::cout << "argolis-demo -h | -v | --version\n";
    std::cout << "argolis-demo [ -c | --count <count> ] [ -a | --adjective [<adjective>] ] <name>...\n";
    std::exit(0);

}

void usage(const argolis::Opt_Spec &, std::optional<std::string_view>)
{
    usage();
}

void version(const argolis::Opt_Spec &, std::optional<std::string_view>)
{
    std::cout << "Argolis demo - version 1.0\n";
    std::exit(0);
}

int main(int argc, char *argv[])
{
    int count = 1;
    std::string adjective;
    
    using namespace argolis;

    Parser parser({
        {'v',"version",Arg_Policy::NO_ARG,version},                 /* this option does not want an argument, it will raise an error if one given   */

        {'h',Arg_Policy::NO_ARG,                                    /* same ( but option only available in "short form"                             */
            [](auto,auto)                                           /* we can safely ignore the arguments to the lambda if they are not needed      */
            {
                usage();
            }
        },
        
        {'c',"count",Arg_Policy::EXPECT_ARG,                        /* this option needs an argument, it will raise an error if none given*/
            [& count](const auto & /* opt */, auto value)           /* type of opt is argolis::Parser::Opt_Spec, type of value is std::optional<std::string_view>   */
            {
                assert(value);                                      // since this option is "NEED_ARG", we expect value to contain an argument

                auto res = std::from_chars(value->data(),value->data()+value->size(),count);
                if( res.ec != std::errc{} || count <= 0 ) {
                    std::cerr << "error: count must be a positive integer value\n";
                    exit(1);
                }

            }
        },
        
        {'a',"adjective",Arg_Policy::MAYBE_ARG,                     /* this option might get an argument, or not    */
            [& adjective](const auto & /* opt */, auto value) {     /* type of opt is argolis::Parser::Opt_Spec, type of value is std::optional<std::string_view>   */
                adjective = value.value_or("dear");
            }
        }
    });
    parser.combi_allowed(true);                                     // allow options in short form to be combined after a dash: "-ac" is equivalent to "-a -c", not to "--ac"

    parser.abort_on_error(true);                                    // abort parsing on the first error

    parser.on_err([](auto err){                                     /* type of err is argolis::Parser::Error        */
        std::cerr << "error with option: " << err.bad_item << "\n";
        std::exit(1);
    });
    
    // NOTE: we capture count by reference otherwise it would not be modified in the lambda above handling the "-c" option
    parser.on_arg([& count, & adjective](std::string_view arg) {
        for( int i = 0; i < count; i++) {
            std::cout << "Hello, " << adjective << ' ' << arg << '\n';
        }
    });
    
    if( argc < 2 ) {
        usage();
    }
    
    parser.parse(argc,argv);

    return 0;
}
