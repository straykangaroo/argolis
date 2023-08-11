#include "argolis.hpp"

#include <iostream>
#include <string_view>
#include <optional>


int main(int argc, char *argv[])
{
    auto on_arg = [](std::string_view arg)
    {
        std::cout << "got arg: " << arg << "\n";
    };

    auto on_err = [](const argolis::Error err)
    {
        switch( err.type ) {
            case argolis::Error::Type::BAD_OPT:         std::cout << "unknown option: ";                    break;
            case argolis::Error::Type::MISSING_ARG:     std::cout << "missing argument for option: ";       break;
            case argolis::Error::Type::UNEXPECTED_ARG:  std::cout << "unexpected argument for option: ";    break;
            default:                                    assert(false);
        }
        std::cout << err.bad_item << "\n";
    };

    auto on_opt = [](const argolis::Opt_Spec & opt, const std::optional<std::string_view> value)
    {
        std::cout << "got opt: " << opt.short_name();
        if( ! opt.long_name().empty() ) {
            std::cout << "/" << opt.long_name();
        }
        std::cout << " with value: " << value.value_or("{}") << "\n";
    };
    
    argolis::Parser p({
        {'a',"all",argolis::Arg_Policy::NO_ARG,on_opt},
        {'v',"verbose",argolis::Arg_Policy::MAYBE_ARG,on_opt},
        {'z',argolis::Arg_Policy::MAYBE_ARG,on_opt},
        {'n',"num",argolis::Arg_Policy::EXPECT_ARG,on_opt}
    });

    p.on_arg(on_arg);
    p.on_err(on_err);

    p.combi_allowed(true);

    p.parse(argc,argv);

    return 0;
}
