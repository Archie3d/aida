with Ada.Text_IO;
procedure Taggedmembership is
    type Root (Enabled : Boolean) is tagged null record;
    subtype On_View is Root (True);
    subtype Off_View is Root (False);
    type Child is new Root with record
        Extra : Integer := 17;
    end record;
    subtype On_Child is Child (True);
    On_Object : Root (True);
    Off_Object : Root (False);
    On_Descendant : Child (True);
    Off_Descendant : Child (False);
    Calls : Integer := 0;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "tagged discriminant membership";
        end if;
    end Check;
    function Identity (Value : Root'Class) return Root'Class is
    begin
        Calls := Calls + 1;
        return Value;
    end Identity;
    procedure Class_Check (Value : Root'Class; Enabled, Exact : Boolean) is
    begin
        Check ((Value in On_View) = (Enabled and Exact));
        Check ((Value not in On_View) = not (Enabled and Exact));
        Check ((Value in Off_View) = (not Enabled and Exact));
        Check ((Value not in Off_View) = not (not Enabled and Exact));
        Check ((Value in On_View'Class) = Enabled);
        Check ((Value not in On_View'Class) = not Enabled);
    end Class_Check;
begin
    Check (On_Object in On_View and On_Object not in Off_View);
    Check (Off_Object not in On_View and Off_Object in Off_View);
    Check (On_Descendant in On_Child and Off_Descendant not in On_Child);
    Class_Check (On_Object, True, True);
    Class_Check (Off_Object, False, True);
    Class_Check (On_Descendant, True, False);
    Class_Check (Off_Descendant, False, False);
    Check (Identity (Off_Object) not in On_View);
    Check (Calls = 1);
    Ada.Text_IO.Put_Line ("tagged membership passed");
end Taggedmembership;
