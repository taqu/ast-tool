namespace typed {

struct Validator {
    void validate() {}
};

struct OtherValidator {
    void validate() {}
};

struct Session {
    Validator validator_;
    Validator* validator_ptr_;
    OtherValidator other_;

    void fieldObject() { validator_.validate(); }
    void fieldPointer() { validator_ptr_->validate(); }
    void unrelatedField() { other_.validate(); }
};

void localObject() {
    Validator validator;
    validator.validate();
}

void referenceParameter(Validator& validator) {
    validator.validate();
}

void pointerParameter(Validator* validator) {
    validator->validate();
}

Validator makeValidator();
void unresolvedExpression() {
    makeValidator().validate();
}

struct Overloaded {
    void validate() {}
    void validate(int) {}
};

struct OverloadSession {
    Overloaded overloaded_;
    void ambiguousMember() { overloaded_.validate(); }
};

} // namespace typed

namespace left {
struct Validator { void validate() {} };
}

namespace right {
struct Validator { void validate() {} };
}

struct NamespaceSession {
    left::Validator left_;
    right::Validator right_;
    void callLeft() { left_.validate(); }
    void callRight() { right_.validate(); }
};

namespace shadow_guard {
struct Validator { void validate() {} };
struct OtherValidator { void validate() {} };

struct Holder {
    Validator receiver;
    void fieldCall() { receiver.validate(); }
};

void localWithFieldName() {
    OtherValidator receiver;
    receiver.validate();
}
} // namespace shadow_guard

namespace scope_guard {
struct Validator { void validate() {} };
struct OtherValidator { void validate() {} };

void siblingBlocks() {
    { Validator receiver; }
    { OtherValidator receiver; receiver.validate(); }
}
} // namespace scope_guard
