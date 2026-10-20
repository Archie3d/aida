procedure ProtectedSyntaxErrors is
    protected type WithEntry is
        entry Wait (1 .. 3);
    end WithEntry;
    protected WithDiscriminant (Value : Integer) is
    end WithDiscriminant;
    protected type WithInterface is new Unknown_Interface with
    end WithInterface;
    protected type WithInstance is
    end WithInstance;
    protected body WithInstance is
        procedure Unsupported is new Unknown_Generic;
    end WithInstance;
    protected type WithOperator is
        function "+" (Value : Integer) return Integer;
    end WithOperator;
    value : Integer := 0;
begin
    goto Unsupported;
end ProtectedSyntaxErrors;
