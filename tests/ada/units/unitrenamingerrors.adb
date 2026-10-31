procedure UnitRenamingErrors is
    Value : Integer := 0;
    package Plain is
        type Item is private;
    private
        Hidden : Integer := 4;
        type Item is record
            Field : Integer;
        end record;
    end Plain;
    generic
    package Generic_Package is
    end Generic_Package;
    generic
    procedure Generic_Procedure;
    procedure Generic_Procedure is
    begin
        null;
    end Generic_Procedure;
    generic
    function Generic_Function return Integer;
    function Generic_Function return Integer is
    begin
        return 1;
    end Generic_Function;
    package Good renames Plain;
    package Wrong_Object renames Value;
    package Wrong_Generic renames Generic_Package;
    package Missing renames Absent;
    package Good renames Plain;
    generic package Wrong_Package renames Plain;
    generic procedure Wrong_Procedure renames Generic_Function;
    generic function Wrong_Function renames Generic_Procedure;
    generic package Wrong_Kind renames Generic_Procedure;
    generic procedure Missing_Generic renames Absent;
    generic package Good_Generic renames Generic_Package;
    generic package Good_Generic renames Generic_Package;
    procedure Wrong_Instance is new Good_Generic;
    Object : Good.Item;
begin
    Value := Good.Hidden;
    Value := Object.Field;
end UnitRenamingErrors;
