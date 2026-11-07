with Report;
package body TCTouch is
    Trace : String (1 .. 1024);
    Used : Natural := 0;
    procedure Assert (Condition : Boolean; Description : String) is
    begin
        if not Condition then
            Report.Failed (Description);
        end if;
    end Assert;
    procedure Assert_Not (Condition : Boolean; Description : String) is
    begin
        Assert (not Condition, Description);
    end Assert_Not;
    procedure Touch (Tag : Character) is
    begin
        if Used = Trace'Length then
            Report.Failed ("TCTouch trace capacity exceeded");
        else
            Used := Used + 1;
            Trace (Used) := Tag;
        end if;
    end Touch;
    procedure Validate (Expected : String; Message : String) is
    begin
        if Trace (1 .. Used) /= Expected then
            Report.Failed (Message);
        end if;
        Used := 0;
    end Validate;
    procedure Validate_One_Of
        (Expected_1, Expected_2, Expected_3, Expected_4, Expected_5, Expected_6 : String;
         Message : String) is
    begin
        if Trace (1 .. Used) /= Expected_1 and then Trace (1 .. Used) /= Expected_2
            and then Trace (1 .. Used) /= Expected_3 and then Trace (1 .. Used) /= Expected_4
            and then Trace (1 .. Used) /= Expected_5 and then Trace (1 .. Used) /= Expected_6 then
            Report.Failed (Message);
        end if;
        Used := 0;
    end Validate_One_Of;
end TCTouch;
