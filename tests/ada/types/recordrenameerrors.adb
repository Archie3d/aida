procedure RecordRenameErrors is
    type Pair is record
        X : Integer;
    end record;
    R : constant Pair := (X => 1);
    X : Integer renames R.X;
    Wrong : Boolean renames R.X;
    Literal : Integer renames 42;
    Expression : Integer renames R.X + 1;
    function Value return Integer is
    begin
        return 1;
    end Value;
    Result : Integer renames Value;
begin
    X := 2;
end RecordRenameErrors;
