procedure ReservedGenericErrors is
    generic
        type Formal is interface;
    package Plain is
    end Plain;
    generic
        type Formal is limited interface;
    package Limited_Formal is
    end Limited_Formal;
    generic
        type Formal is synchronized interface;
    package Synchronized_Formal is
    end Synchronized_Formal;
    generic
        type Formal is protected interface;
    package Protected_Formal is
    end Protected_Formal;
    generic
        type Formal is access protected procedure;
    package Callback_Formal is
    end Callback_Formal;
begin
    null;
end ReservedGenericErrors;
