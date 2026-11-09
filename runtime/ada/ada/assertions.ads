package Ada.Assertions is
    pragma Pure;
    Assertion_Error : exception;
    procedure Assert (Check : Boolean);
    procedure Assert (Check : Boolean; Message : String);
end Ada.Assertions;
