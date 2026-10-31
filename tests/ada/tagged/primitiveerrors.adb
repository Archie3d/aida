procedure PrimitiveErrors is
    package P is
        type T is range 0 .. 10;
        function Read (X : T) return Integer;
        procedure Set (X : out T);
        type Early is new T;
        function Late (X : T) return Integer;
    private
        function Secret (X : T) return Integer;
    end P;
    package Q is
        type T is new P.T;
        overriding function Missing (X : T) return Integer;
        not overriding function Read (X : T) return Integer;
        overriding procedure Set (X : in out T);
    end Q;
    use type P.T;
    X : P.T := 1;
    Y : Q.T := 1;
    E : P.Early := 1;
    A : Integer := Read (X);
    B : Integer := P.Late (E);
    C : Integer := Q.Secret (Y);
    type Callback is access function (X : P.Early) return Integer;
    F : Callback := P.Read'Access;
    D : P.T := Y;
    use all type Q.T;
    S : Integer := Secret (Y);
    use all type P.T;
    Hidden : Integer := Secret (X);
    Selected : Integer := P.Secret (X);
begin
    null;
end PrimitiveErrors;
