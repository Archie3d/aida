procedure VisibilityErrors is
    package A is
        Value : Integer := 1;
        type Item is range 1 .. 10;
        function Get return Integer;
    private
        Secret : Integer := 2;
        Hidden : exception;
        subtype Private_Item is Integer;
    end A;
    package body A is
        function Get return Integer is begin return Secret; end Get;
        Body_Only : Integer := 3;
    end A;
    package B is
        Value : Integer := 2;
        type Item is range 1 .. 10;
        function Get return Integer;
    end B;
    package body B is
        function Get return Integer is begin return 2; end Get;
    end B;
    use A, B;
    X : Integer := Value;
    Y : Item;
    Z : Integer := Get;
    Secret_Copy : Integer := A.Secret;
    Private_Copy : A.Private_Item;
    Body_Copy : Integer := A.Body_Only;
    use X;
begin
    raise A.Hidden;
end VisibilityErrors;
