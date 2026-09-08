package com.example;

public interface Greeter {
    String greet(String name);
    default void log() {}
}

public @interface MyAnnotation {
    String value() default "";
}

public enum Status {
    ACTIVE, INACTIVE, PENDING;
    private final int code;
    Status(int code) { this.code = code; }
    public int getCode() { return code; }
}

public class Service implements Greeter {
    public static final int MAX = 100;
    private String host;

    public Service(String host) { this.host = host; }

    @Override
    public String greet(String name) { return "Hello " + name; }

    public static class Builder {
        private String host;
        public Builder host(String h) { this.host = h; return this; }
        public Service build() { return new Service(host); }
    }
}
