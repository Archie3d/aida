procedure GenericLiteralErrors is
    type Color is (Red, Green);
    type Other is (Blue, Yellow);
    type Glyph is ('A', 'a');
    generic
        with function Pick return Color;
    package Views is
        function Value return Color renames Pick;
    end Views;
    generic
        with function Pick (X : Integer) return Color;
    package Unary is end Unary;
    generic
        with procedure Pick;
    package Actions is end Actions;
    package Wrong_Result is new Views (Blue);
    package Wrong_Arity is new Unary (Red);
    package Wrong_Kind is new Actions (Red);
    Object : Color := Red;
    package Not_Callable is new Views (Object);
    package Numeric is new Views (1);
    package Expression_Actual is new Views (Color'(Red));
    generic
        with function Pick return Color is 'a';
    package Bad_Default is end Bad_Default;
    generic
        with function Pick (X : Integer) return Color is Red;
    package Bad_Default_Arity is end Bad_Default_Arity;
    package Bound is new Views (Red);
    type Callback is access function return Color;
    Pointer : Callback := Bound.Value'Access;
    package Forwarded is new Views (Bound.Value);
    Pointer_2 : Callback := Forwarded.Value'Access;
    type Narrow is new Color range Green .. Green;
    Outside : Narrow := Narrow (Color'(Red));
    generic
        with procedure Pick is Red;
    package Bad_Procedure_Default is end Bad_Procedure_Default;
begin
    null;
end GenericLiteralErrors;
