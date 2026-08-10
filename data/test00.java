/**
 * Module-level documentation for Coding Agent testing.
 */
package com.agent.test;

import java.util.Map;
import java.util.Optional;

/**
 * Manages user session and authentication.
 */
class UserManager {
    private final String dbUrl;

    public UserManager(String dbUrl) {
        this.dbUrl = dbUrl;
    }

    public Optional<Map<String, String>> getUserById(int userId) {
        // TODO: Implement database lookup
        return Optional.empty();
    }
}

public class Main {
    public static void main(String[] args) {
        // Java 14+ 新しい switch 式/パターンのテスト
        int status = 200;
        switch (status) {
            case 200, 201 -> System.out.println("Success");
            default -> System.out.println("Failure");
        }
    }
}
