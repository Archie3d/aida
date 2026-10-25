package body Rejection_Model.Hidden is
    function Join return Name is
        First : Name := "abcd";
        Last : Name := "WXYZ";
    begin
        return First (1 .. 2) & Last (3 .. 4);
    end Join;
end Rejection_Model.Hidden;
