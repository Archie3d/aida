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
    procedure Flush is
    begin
        Used := 0;
    end Flush;
    procedure Validate (Expected : String; Message : String;
                        Order_Meaningful : Boolean := True) is
        Matches : Boolean := Trace (1 .. Used) = Expected;
        Actual_Count, Expected_Count : Natural;
    begin
        if not Order_Meaningful and then Used = Expected'Length then
            Matches := True;
            for Tag in Character loop
                Actual_Count := 0;
                Expected_Count := 0;
                for Index in 1 .. Used loop
                    if Trace (Index) = Tag then
                        Actual_Count := Actual_Count + 1;
                    end if;
                    if Expected (Expected'First + Index - 1) = Tag then
                        Expected_Count := Expected_Count + 1;
                    end if;
                end loop;
                if Actual_Count /= Expected_Count then
                    Matches := False;
                end if;
            end loop;
        end if;
        if not Matches then
            Report.Failed (Message);
        end if;
        Used := 0;
    end Validate;
    procedure Validate_One_Of
        (Expected_1, Expected_2 : String;
         Expected_3, Expected_4, Expected_5, Expected_6 : String := "";
         Message : String) is
    begin
        if Trace (1 .. Used) /= Expected_1 and then Trace (1 .. Used) /= Expected_2
            and then (Expected_3 = "" or else Trace (1 .. Used) /= Expected_3)
            and then (Expected_4 = "" or else Trace (1 .. Used) /= Expected_4)
            and then (Expected_5 = "" or else Trace (1 .. Used) /= Expected_5)
            and then (Expected_6 = "" or else Trace (1 .. Used) /= Expected_6) then
            Report.Failed (Message);
        end if;
        Used := 0;
    end Validate_One_Of;
end TCTouch;
