#!/usr/bin/env bash
# ============================================================
# 对照检查：我们的 User/ 和参考工程差多少
#
# 用法（在仓库任意位置都能跑）：
#   study_workstation/tools/ref_diff.sh              和实机参考对比（默认）
#   study_workstation/tools/ref_diff.sh --pc         和参考工程的 PC 版对比
#   study_workstation/tools/ref_diff.sh -f HWData    只看一个文件的完整 diff（干净版）
#   study_workstation/tools/ref_diff.sh -f HWData --raw  同一份 diff，但保留注释和原排版
#   study_workstation/tools/ref_diff.sh -h           帮助
#
# 默认是「代码级」比较：先去掉注释、空行，再把连续空白压成一个空格，
# 这样缩进、换行、注释造成的噪音不会盖住真正的差异。
# 「重合」这一列 = 两边一模一样的行 / 行数多的那一边，100% 就是一字不差。
#
# 最该盯的一列是「接口(缺/多)」：
#   缺 = 参考有、我们没有的函数   → 应该永远是 0
#   多 = 我们有、参考没有的函数   → 每一个都要在 course/对照表.md 里写清理由
# ============================================================
set -u

TMPD="$(mktemp -d)"
trap 'rm -rf "$TMPD"' EXIT

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
OURS="$ROOT/study_workstation/ovwatch_learn/User"
REF_FW="$ROOT/Software/OV_Watch/User"
REF_PC="$ROOT/lv_sim_vscode_win/user_test"

# 参考里有、我们直接复用（或根本不需要）的资源，不参与对比
IGNORE_RE='^GUI_App/(Fonts|IMGs)/|^version\.h$'

MODE=fw
RAW=0
ONEFILE=""

usage() {
    awk 'NR>1 { if ($0 !~ /^#/) exit; sub(/^# ?/, ""); print }' "$0"
    exit 0
}

while [ $# -gt 0 ]; do
    case "$1" in
        --pc)      MODE=pc ;;
        --raw)     RAW=1 ;;
        -f|--file) shift; ONEFILE="${1:-}" ;;
        -h|--help) usage ;;
        *) echo "未知参数: $1（用 -h 看用法）"; exit 2 ;;
    esac
    shift
done

if [ "$MODE" = pc ]; then
    REF="$REF_PC"
else
    REF="$REF_FW"
fi

[ -d "$OURS" ] || { echo "找不到我们的代码目录: $OURS"; exit 1; }
[ -d "$REF" ]  || { echo "找不到参考目录: $REF"; exit 1; }

# ------------------------------------------------------------
# 工具函数
# ------------------------------------------------------------

# 去掉注释和空行，压缩空白（C 字符串里的 // 也会被切掉，这里够用）
norm() {
    awk '
    {
        line = $0
        sub(/\r$/, "", line)
        out = ""; i = 1; n = length(line)
        while (i <= n) {
            two = substr(line, i, 2)
            if (inblock) {
                if (two == "*/") { inblock = 0; i += 2 } else { i++ }
            } else if (two == "/*") {
                inblock = 1; i += 2
            } else if (two == "//") {
                break
            } else {
                out = out substr(line, i, 1); i++
            }
        }
        gsub(/[ \t]+/, " ", out)
        sub(/^ /, "", out); sub(/ $/, "", out)
        if (out != "") print out
    }' "$1"
}

# 在 norm 的基础上再把标点两边的空格去掉，专供计数用
# （`lv_obj_t * a` 和 `lv_obj_t *a` 才算同一行；但这么写人看着累，所以 -f 不用它）
tnorm() {
    norm "$1" | sed 's/ *\([*(),;{}[\]]\) */\1/g'
}

# 从函数定义里抽函数名（兼容 "{" 换行和同行两种写法）
funcs() {
    norm "$1" | awk '
    function extract(h,   s, name, t) {
        if (h ~ /^[ \t]*(if|for|while|switch|else|do)[ \t(]/) return ""
        if (h ~ /[;=]/) return ""
        if (h !~ /\(/) return ""
        s = h; name = ""
        while (match(s, /[A-Za-z_][A-Za-z0-9_]*[ \t]*\(/)) {
            t = substr(s, RSTART, RLENGTH)
            sub(/[ \t]*\($/, "", t)
            name = t
            s = substr(s, RSTART + RLENGTH)
        }
        return name
    }
    {
        line = $0
        if (line == "{") {
            if (pend != "") { n = extract(pend); if (n != "") print n; pend = "" }
            next
        }
        if (line ~ /\{[ \t]*$/) {
            h = line; sub(/[ \t]*\{[ \t]*$/, "", h)
            n = extract(h); if (n != "") print n
            pend = ""
            next
        }
        if (line ~ /\)[ \t]*$/) pend = line
        else pend = ""
    }'
}

list_files() {
    ( cd "$1" && find . -type f \( -name '*.c' -o -name '*.h' \) | sed 's|^\./||' | sort )
}

# 行级重合度：两边一模一样的行（按行内容，顺序无关）/ 行数多的一边
# 用多重集求交，所以重复的空行、花括号也会各自配对，不会虚高
overlap_pct() {
    local a="$TMPD/a" b="$TMPD/b" ra rb common
    tnorm "$1" > "$a"
    tnorm "$2" > "$b"
    ra=$(wc -l < "$a"); rb=$(wc -l < "$b")
    if [ "$ra" -eq 0 ] && [ "$rb" -eq 0 ]; then echo 100; return; fi
    common=$(comm -12 <(sort "$a") <(sort "$b") | wc -l | tr -d ' ')
    if [ "$ra" -ge "$rb" ]; then echo $((common * 100 / ra)); else echo $((common * 100 / rb)); fi
}

# 两个文件是否一字不差（代码级）
is_identical() {
    tnorm "$1" > "$TMPD/a"
    tnorm "$2" > "$TMPD/b"
    cmp -s "$TMPD/a" "$TMPD/b"
}

show_diff() {
    if [ "$RAW" = 1 ]; then
        diff --strip-trailing-cr -u "$1" "$2"
    else
        local a b
        a="$(mktemp)"; b="$(mktemp)"
        norm "$1" > "$a"
        norm "$2" > "$b"
        diff -u "$a" "$b"
        rm -f "$a" "$b"
    fi
}

# ------------------------------------------------------------
# 单文件模式
# ------------------------------------------------------------
if [ -n "$ONEFILE" ]; then
    # 这版 Git Bash 的 grep -i 配上 -F 会崩，用 awk 做忽略大小写的子串匹配
    hits="$(list_files "$REF" | awk -v pat="$(printf '%s' "$ONEFILE" | tr 'A-Z' 'a-z')" \
        'index(tolower($0), pat) > 0')"
    [ -n "$hits" ] || { echo "参考里找不到匹配 '$ONEFILE' 的文件"; exit 1; }
    for f in $hits; do
        echo "============================================================"
        echo "参考: $REF/$f"
        if [ -f "$OURS/$f" ]; then
            echo "文件: $OURS/$f"
            echo "============================================================"
            show_diff "$REF/$f" "$OURS/$f"
        else
            echo "我们还没有这个文件"
        fi
    done
    exit 0
fi

# ------------------------------------------------------------
# 全量清单
# ------------------------------------------------------------
if [ "$MODE" = pc ]; then
    echo "参考基线：lv_sim_vscode_win/user_test（参考工程的 PC 版）"
else
    echo "参考基线：Software/OV_Watch/User（实机版，权威基线）"
fi
echo "比较方式：代码级（去注释 / 空行 / 缩进噪音；--raw 只影响 -f 的显示）"
echo "已忽略：GUI_App/Fonts、GUI_App/IMGs、version.h（直接复用参考工程）"
echo

# 中文占两列，printf 只按字节补空格，所以表头自己按显示宽度补
# （一个汉字 3 字节、占 2 列，于是 显示宽度 = 字节数 - 非 ASCII 字节数 / 3）
hpad() {
    local s="$1" target="$2" align="${3:-l}" bytes nonascii disp spaces
    # ${#s} 在 UTF-8 下数字符，这里要的是字节数，所以走 wc -c
    bytes=$(printf '%s' "$s" | wc -c | tr -d ' ')
    nonascii=$(printf '%s' "$s" | LC_ALL=C tr -d '\000-\177' | wc -c | tr -d ' ')
    disp=$((bytes - nonascii / 3))
    spaces=$((target - disp)); [ "$spaces" -lt 0 ] && spaces=0
    if [ "$align" = r ]; then printf '%*s%s' "$spaces" '' "$s"
    else printf '%s%*s' "$s" "$spaces" ''; fi
}
printf '%s %s %s %s %s %s\n' \
    "$(hpad 状态 4)" "$(hpad 文件 44)" "$(hpad 参考行 7 r)" \
    "$(hpad 我们行 7 r)" "$(hpad 重合 8 r)" "$(hpad 接口缺/多 9 r)"
printf '%s %s %s %s %s %s\n' \
    "----" "--------------------------------------------" "-------" "-------" "--------" "---------"

common=0; same=0; only_ref=0; only_ours=0
missing_total=0; extra_total=0
missing_list=""; extra_list=""
rows=""

for f in $( { list_files "$REF"; list_files "$OURS"; } | sort -u ); do
    # 参考资源直接跳过
    if [ ! -f "$OURS/$f" ] && echo "$f" | grep -Eq "$IGNORE_RE"; then
        continue
    fi

    if [ -f "$REF/$f" ] && [ -f "$OURS/$f" ]; then
        common=$((common + 1))
        nref=$(wc -l < "$REF/$f" | tr -d ' ')
        nour=$(wc -l < "$OURS/$f" | tr -d ' ')
        pct="$(overlap_pct "$REF/$f" "$OURS/$f")%"
        miss=$(comm -23 <(funcs "$REF/$f" | sort -u) <(funcs "$OURS/$f" | sort -u) | sed '/^$/d')
        extra=$(comm -13 <(funcs "$REF/$f" | sort -u) <(funcs "$OURS/$f" | sort -u) | sed '/^$/d')
        nmiss=$(printf '%s' "$miss" | grep -c . )
        nextra=$(printf '%s' "$extra" | grep -c . )
        missing_total=$((missing_total + nmiss))
        extra_total=$((extra_total + nextra))
        if [ -n "$miss" ]; then missing_list="${missing_list}${f}: $(echo $miss)
"; fi
        if [ -n "$extra" ]; then extra_list="${extra_list}${f}: $(echo $extra)
"; fi

        if is_identical "$REF/$f" "$OURS/$f"; then
            st="[=]"; same=$((same + 1))
        else
            st="[~]"
        fi
        rows="${rows}${st}|${f}|${nref}|${nour}|${pct}|${nmiss}/${nextra}
"
    elif [ -f "$REF/$f" ]; then
        only_ref=$((only_ref + 1))
        nref=$(wc -l < "$REF/$f" | tr -d ' ')
        rows="${rows}[ ]|${f}|${nref}|-|-|-
"
    else
        only_ours=$((only_ours + 1))
        nour=$(wc -l < "$OURS/$f" | tr -d ' ')
        rows="${rows}[+]|${f}|-|${nour}|-|-
"
    fi
done

# [~] 的排在前面（重合度低的优先看），然后 [=]，然后未做、新增
echo "$rows" | grep -v '^$' | awk -F'|' '
    { print ($1 == "[~]" ? 0 : $1 == "[=]" ? 1 : $1 == "[ ]" ? 2 : 3) "\t" $5 "\t" $0 }
' | sort -t$'\t' -k1,1n -k2,2n | cut -f3- | while IFS='|' read -r st f nref nour pct nint; do
    printf '%-4s %-44s %7s %7s %8s %9s\n' "$st" "$f" "$nref" "$nour" "$pct" "$nint"
done

echo
echo "图例：[=] 代码相同   [~] 有差异   [ ] 还没做   [+] 我们新增的"
echo "汇总：共同 $common 个（完全相同 $same 个），未做 $only_ref 个，新增 $only_ours 个"
echo "接口：缺失 $missing_total 个函数，多出 $extra_total 个函数"

if [ "$missing_total" -gt 0 ]; then
    echo
    echo "⚠ 以下函数参考里有、我们没有（这属于「接口漂移」，要么补上，要么在对照表里写明）："
    printf '%s' "$missing_list"
fi

if [ "$extra_total" -gt 0 ]; then
    echo
    echo "我们多出来的函数（教学拆分、仿真源、桩，都应在对照表里有解释）："
    printf '%s' "$extra_list"
fi

echo
echo "下一步：把变化记进 study_workstation/course/对照表.md（每课收尾跑一次）。"
