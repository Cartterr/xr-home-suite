#pragma once

namespace xrh {

class ShutdownSignal {
public:
    bool install();
    void uninstall();
    bool requested() const;
    void reset();

private:
    bool installed_{false};
};

}  // namespace xrh
