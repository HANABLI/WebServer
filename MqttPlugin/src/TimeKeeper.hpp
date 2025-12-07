#ifndef WEBSERVER_TIMEKEEPER_HPP
#define WEBSERVER_TIMEKEEPER_HPP
/**
 * @file TimeKeeper.hpp
 *
 * This module declares the TimeKeeper definition needed to keep
 * time for mqttClient, it implements MqttV5::TimeKeeper interface.
 *
 * © 2025 by Hatem Nabli
 */

#include <MqttV5/TimeKeeper.hpp>
#include <memory>

/**
 * This represents the time-keeping requirements of MqttV5::Server.
 * To integrate MqttV5::Server into a large program, implement this
 * interface in terms of the actual server time.
 *
 */
class TimeKeeper : public MqttV5::TimeKeeper
{
    // Life cycle Managment
public:
    ~TimeKeeper() noexcept;
    TimeKeeper(const TimeKeeper&) = delete;  // Copy Constructor that creates a new object by making
                                             // a copy of an existing object.
    // It ensures that a deep copy is performed if the object contains dynamically allocated
    // resources
    TimeKeeper(TimeKeeper&&);  // Move Constructor that transfers resources from an expiring object
                               // to a newly constructed object.
    TimeKeeper& operator=(const TimeKeeper&) =
        delete;  // Copy Assignment Operation That assigns the values of one object to another
                 // object using the assignment operator (=)
    TimeKeeper& operator=(
        TimeKeeper&&);  // Move Assignment Operator: Amove assignment operator efficiently transfers
                        // resources from one object to another.

public:
    /**
     * This is the default constructor
     */
    TimeKeeper();

    // Methods

public:
    virtual double GetCurrentTime() override;

private:
    /**
     * This is the type of structure that contains the private
     * properties of the instance. It is defined in the implementation
     * and declared here to ensure that iwt is scoped inside the class.
     */
    struct Impl;
    /**
     * This contains the private properties of the instance.
     */
    std::unique_ptr<struct Impl> _impl;
};

#endif /* WEBSERVER_TIMEKEEPER_HPP */