procedure MissingBodyErrors is
    procedure Never_Completed;
    function Missing_Function return Integer;
    procedure Nested is
        procedure Also_Missing;
    begin
        null;
    end Nested;
begin
    declare
        procedure Missing_In_Block;
    begin
        null;
    end;
end MissingBodyErrors;

package Missing_Local_Body is
end Missing_Local_Body;

package body Missing_Local_Body is
    procedure Missing_In_Package_Body;
end Missing_Local_Body;
