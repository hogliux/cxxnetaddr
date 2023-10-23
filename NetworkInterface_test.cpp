//
//  NetworkInterface_test.cpp
//  cxxnetaddr - https://github.com/hogliux/cxxnetaddr
//
//  Created by Fabian Renn-Giles, fabian@fieldingdsp.com on 18th September 2022.
//  Copyright © 2022 Fielding DSP GmbH, All rights reserved.
//
//  Fielding DSP GmbH
//  Jägerstr. 36
//  14467 Potsdam, Germany
//
#include <gtest/gtest.h>
#include "NetworkInterface.hpp"

// Test creating an invalid NetworkInterface
TEST(NetworkInterfaceTest, InvalidInterface) {
    NetworkInterface intf;
    EXPECT_FALSE(intf.isValid());
}

// Test comparison operators
TEST(NetworkInterfaceTest, InterfaceComparison) {
    NetworkInterface intf1;
    NetworkInterface intf2;
    EXPECT_EQ(intf1, intf2);
}

// Since we cannot rely on actual interfaces, we test the copy and move constructors
TEST(NetworkInterfaceTest, CopyAndMoveSemantics) {
    NetworkInterface intf1;
    NetworkInterface intf2(intf1); // Copy constructor
    EXPECT_EQ(intf1, intf2);

    NetworkInterface intf3(std::move(intf1)); // Move constructor
    EXPECT_EQ(intf3, intf2);
    EXPECT_FALSE(intf1.isValid()); // intf1 should now be invalid
}

// Test the getType method for an unknown type
TEST(NetworkInterfaceTest, GetTypeUnknown) {
    NetworkInterface intf;
    EXPECT_EQ(intf.getType(), NetworkInterface::Type::unknown);
}

// Test that isValid returns true when a valid interface is provided (simulated)
TEST(NetworkInterfaceTest, ValidInterfaceSimulation) {
    // Simulate a valid interface by using the fromString method with a known interface name
    // Since we cannot rely on actual interfaces, we'll assume "eth0" is a valid name
    // Note: In actual unit tests, you might mock the fromString method
    auto intfOpt = NetworkInterface::fromString("eth0");
    if (intfOpt.has_value()) {
        auto intf = intfOpt.value();
        EXPECT_TRUE(intf.isValid());
        EXPECT_EQ(intf.getName(), "eth0");
    } else {
        SUCCEED() << "Interface 'eth0' not found. Skipping test.";
    }
}

// Test that getIndex returns 0 for an invalid interface
TEST(NetworkInterfaceTest, GetIndexInvalidInterface) {
    NetworkInterface intf;
    EXPECT_EQ(intf.getIndex(), 0u);
}

// Test that getName returns an empty string for an invalid interface
TEST(NetworkInterfaceTest, GetNameInvalidInterface) {
    NetworkInterface intf;
    EXPECT_EQ(intf.getName(), "");
}
