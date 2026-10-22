with Report;
package body TCTouch is
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
end TCTouch;
