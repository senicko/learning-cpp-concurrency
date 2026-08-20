#include <string>

// Example of an Actor Model / message passing style of programming in CPP. The
// atm class models a state machine.

struct card_inserted {
    std::string account;
};

class atm {
    messaging::receiver incoming;
    messaging::sender back;
    messaging::sender interface_hardware;

    // This is a pointer to a member function of the atm class. This is a bit
    // different from a normal function pointer. It can't be invoked on it's own
    // but rather on an instance like (this->*state)();
    void (atm::*state)();

    std::string account;
    std::string pin;

    void waiting_for_card()
    {
        interface_hardware.send(display_enter_card());

        incoming.wait().handle<card_inserted>([&](card_inserted const& msg) {
            account = msg.account;
            pin = "";
            interface_hardware.send(display_enter_pin());
            state = &atm::getting_pin;
        });
    }

    void getting_pin()
    {
        incoming.wait()
            .handle<digit_pressed>([&](digit_pressed const& msg) {
                unsigned const pin_length = 4;
                pin += msg.digit;
                if (pin.length() == pin_length) {
                    bank.send(verify_pin(account, pin, incoming));
                    state = &atm::verifying_pin;
                }
            })
            .handle<clear_last_pressed>([&](clear_last_pressed const& msg) {
                if (!pin.empty()) {
                    pin.resize(pin.length() - 1);
                }
            })
            .handle<cancel_pressed>([&](cancel_pressed const& msg) {
                state = &atm::done_processing;
            });
    }

public:
    void run()
    {
        state = &atm::waiting_for_card;

        try {
            for (;;) {
                (this->*state)();
            }
        } catch (messaging::close_queue const&) {
        }
    }
};

int main() { return 0; }
