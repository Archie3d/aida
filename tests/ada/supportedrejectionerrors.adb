with Rejection_Model;
procedure SupportedRejectionErrors is
    type Vector is array (1 .. 3) of Integer;
    type Other_Vector is array (1 .. 3) of Integer;
    function Fetch return Vector is
    begin
        return (1, 2, 3);
    end Fetch;
    function Fetch (Index : Integer) return Integer is
    begin
        return Index;
    end Fetch;
    function Same return Vector is
    begin
        return (1, 2, 3);
    end Same;
    function Same return Other_Vector is
    begin
        return (1, 2, 3);
    end Same;
    procedure Change (Value : in out Integer) is
    begin
        Value := 0;
    end Change;
    function Only return Vector is
    begin
        return (1, 2, 3);
    end Only;
    Data : Vector := (1, 2, 3);
    Named_Object_Index : Integer := Data (Position => 1);
    Value : Integer := Fetch (1);
    Ambiguous_Prefix : Integer := Same (1);
    Named_Index : Integer := Same (Position => 1);
    Wrong_Rank : Integer := Same (1, 2);
    Wrong_Index : Integer := Same (True);
    package Outer_Package is
        package Inner_Package is end Inner_Package;
    end Outer_Package;
    package body Outer_Package is
        Body_Local : Integer := 1;
        package body Inner_Package is
        begin
            Body_Local := 2;
        end Inner_Package;
    end Outer_Package;
    Leak : Integer := Outer_Package.Body_Local;
    Item : Rejection_Model.Item (2) := Rejection_Model.Create (2);
    Hidden_Field : Integer := Item.Hidden;
    Secret : Rejection_Model.Secret := Rejection_Model.Make_Secret;
    Hidden_Discriminant : Integer := Secret.Key;
    Hidden_Array : Rejection_Model.Name;
    Copied : Rejection_Model.Name := Hidden_Array & Hidden_Array;
    package Truly_Limited is
        type Cell is limited record Value : Integer; end record;
        type Cells is array (1 .. 2) of Cell;
        Left, Right : Cells;
        Joined : Cells := Left & Right;
    end Truly_Limited;
begin
    Change (Only (1));
    Item.Length := 4;
end SupportedRejectionErrors;
