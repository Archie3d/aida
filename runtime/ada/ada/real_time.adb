package body Ada.Real_Time is
    -- Keep the runtime ABI in concrete scalar types. Public private formals
    -- retain the compiler's Ada pointer ABI through their full completion.
    function Duration_Value (span : Long_Integer) return Duration;
    pragma Import (C, Duration_Value, "__ada_real_to_duration");
    function Span_Value (d : Duration) return Long_Integer;
    pragma Import (C, Span_Value, "__ada_real_to_span");
    procedure Split_Value (t : Long_Integer; sc, ts : out Long_Integer);
    pragma Import (C, Split_Value, "__ada_real_split");
    function Compose (sc, ts : Long_Integer) return Long_Integer;
    pragma Import (C, Compose, "__ada_real_time_of");

    function To_Duration (ts : Time_Span) return Duration is
    begin
        return Duration_Value (Long_Integer (ts));
    end To_Duration;

    function To_Time_Span (d : Duration) return Time_Span is
    begin
        return Time_Span (Span_Value (d));
    end To_Time_Span;

    procedure Split (t : Time; sc : out Seconds_Count; ts : out Time_Span) is
        seconds, remainder : Long_Integer;
    begin
        Split_Value (Long_Integer (t), seconds, remainder);
        sc := Seconds_Count (seconds);
        ts := Time_Span (remainder);
    end Split;

    function Time_Of (sc : Seconds_Count; ts : Time_Span) return Time is
    begin
        return Time (Compose (Long_Integer (sc), Long_Integer (ts)));
    end Time_Of;

    function "+" (left : Time; right : Time_Span) return Time is
    begin
        return Time (Long_Integer (left) + Long_Integer (right));
    end;

    function "+" (left : Time_Span; right : Time) return Time is
    begin
        return Time (Long_Integer (left) + Long_Integer (right));
    end;

    function "-" (left : Time; right : Time_Span) return Time is
    begin
        return Time (Long_Integer (left) - Long_Integer (right));
    end;

    function "-" (left, right : Time) return Time_Span is
    begin
        return Time_Span (Long_Integer (left) - Long_Integer (right));
    end;

    function "<" (left, right : Time) return Boolean is
    begin
        return Long_Integer (left) < Long_Integer (right);
    end;

    function "<=" (left, right : Time) return Boolean is
    begin
        return Long_Integer (left) <= Long_Integer (right);
    end;

    function ">" (left, right : Time) return Boolean is
    begin
        return Long_Integer (left) > Long_Integer (right);
    end;

    function ">=" (left, right : Time) return Boolean is
    begin
        return Long_Integer (left) >= Long_Integer (right);
    end;

    function "<" (left, right : Time_Span) return Boolean is
    begin
        return Long_Integer (left) < Long_Integer (right);
    end;

    function "<=" (left, right : Time_Span) return Boolean is
    begin
        return Long_Integer (left) <= Long_Integer (right);
    end;

    function ">" (left, right : Time_Span) return Boolean is
    begin
        return Long_Integer (left) > Long_Integer (right);
    end;

    function ">=" (left, right : Time_Span) return Boolean is
    begin
        return Long_Integer (left) >= Long_Integer (right);
    end;

    function "+" (left, right : Time_Span) return Time_Span is
    begin
        return Time_Span (Long_Integer (left) + Long_Integer (right));
    end;

    function "-" (left, right : Time_Span) return Time_Span is
    begin
        return Time_Span (Long_Integer (left) - Long_Integer (right));
    end;

    function "-" (right : Time_Span) return Time_Span is
    begin
        return Time_Span (-Long_Integer (right));
    end;

    function "*" (left : Time_Span; right : Integer) return Time_Span is
    begin
        return Time_Span (Long_Integer (left) * Long_Integer (right));
    end;

    function "*" (left : Integer; right : Time_Span) return Time_Span is
    begin
        return Time_Span (Long_Integer (left) * Long_Integer (right));
    end;

    function "/" (left, right : Time_Span) return Integer is
    begin
        return Integer (Long_Integer (left) / Long_Integer (right));
    end;

    function "/" (left : Time_Span; right : Integer) return Time_Span is
    begin
        return Time_Span (Long_Integer (left) / Long_Integer (right));
    end;

    function "abs" (right : Time_Span) return Time_Span is
    begin
        return Time_Span (abs Long_Integer (right));
    end;

    function Nanoseconds (ns : Integer) return Time_Span is
    begin
        return Time_Span (Long_Integer (ns) * 1);
    end Nanoseconds;

    function Microseconds (us : Integer) return Time_Span is
    begin
        return Time_Span (Long_Integer (us) * 1000);
    end Microseconds;

    function Milliseconds (ms : Integer) return Time_Span is
    begin
        return Time_Span (Long_Integer (ms) * 1000000);
    end Milliseconds;

    function Seconds (s : Integer) return Time_Span is
    begin
        return Time_Span (Long_Integer (s) * 1000000000);
    end Seconds;

    function Minutes (m : Integer) return Time_Span is
    begin
        return Time_Span (Long_Integer (m) * 60000000000);
    end Minutes;

end Ada.Real_Time;
