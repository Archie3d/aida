procedure AbstractCallErrors is
    package Model is
        type Root is abstract tagged record
            Value : Integer := 0;
        end record;
        procedure Act (Self : Root) is abstract;
        function Read (Self : Root) return Integer is abstract;
        function Make return Root is abstract;
        function Wide return Root'Class is abstract;
        procedure Defaulted (Self : Root := Make) is null;
        function "+" (Self : Root) return Root is abstract;
    end Model;
    procedure Check (Self : in out Model.Root) is
        use type Model.Root;
        procedure Alias (Self : Model.Root) renames Model.Act;
        Number : Integer := Model.Read (Self);
        Wide : Model.Root'Class := Model.Wide;
    begin
        Model.Act (Self);
        Alias (Self);
        Model.Defaulted;
        Model.Act (Model.Make);
        Model.Act (+Self);
    end Check;
begin
    null;
end AbstractCallErrors;
