package body Ada.Assertions is
    procedure Assert (Check : Boolean) is
    begin
        if not Check then
            raise Assertion_Error;
        end if;
    end Assert;
    procedure Assert (Check : Boolean; Message : String) is
    begin
        if not Check then
            raise Assertion_Error with Message;
        end if;
    end Assert;
end Ada.Assertions;
