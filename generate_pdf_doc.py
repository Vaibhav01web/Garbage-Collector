#!/usr/bin/env python3
"""
Generate a comprehensive, publication-grade PDF documentation for the
Memory Compaction & Garbage Collector Engine project using ReportLab.
"""

import os
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.units import inch
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable, Preformatted
)
from reportlab.pdfgen import canvas

PDF_OUTPUT_PATH = "Memory_Compaction_and_GC_Engine_Documentation.pdf"

class NumberedCanvas(canvas.Canvas):
    """Two-pass canvas to dynamically compute and print 'Page X of Y' in footer."""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super().showPage()
        super().save()

    def draw_page_decorations(self, page_count):
        self.saveState()
        
        # Don't draw header on first page (cover)
        if self._pageNumber > 1:
            # Header rule & text
            self.setStrokeColor(colors.HexColor("#cbd5e1"))
            self.setLineWidth(0.5)
            self.line(54, letter[1] - 40, letter[0] - 54, letter[1] - 40)
            
            self.setFont("Helvetica-Bold", 8)
            self.setFillColor(colors.HexColor("#64748b"))
            self.drawString(54, letter[1] - 34, "MEMORY COMPACTION & GC ENGINE — SYSTEM SPECIFICATION")
            self.drawRightString(letter[0] - 54, letter[1] - 34, "C++ RUNTIME ENGINE")

        # Footer rule & text (on all pages)
        self.setStrokeColor(colors.HexColor("#e2e8f0"))
        self.setLineWidth(0.5)
        self.line(54, 46, letter[0] - 54, 46)

        self.setFont("Helvetica", 8)
        self.setFillColor(colors.HexColor("#64748b"))
        self.drawString(54, 32, "Confidential & Proprietary • In-Memory Garbage Collection Engine")
        page_str = f"Page {self._pageNumber} of {page_count}"
        self.drawRightString(letter[0] - 54, 32, page_str)

        self.restoreState()


def build_pdf():
    doc = SimpleDocTemplate(
        PDF_OUTPUT_PATH,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=54,
        bottomMargin=54
    )

    styles = getSampleStyleSheet()
    
    # Custom palette
    c_primary = colors.HexColor("#0f172a")     # Slate 900
    c_secondary = colors.HexColor("#334155")   # Slate 700
    c_blue = colors.HexColor("#1d4ed8")        # Blue 700
    c_indigo = colors.HexColor("#4338ca")      # Indigo 700
    c_cyan = colors.HexColor("#0e7490")        # Cyan 700
    c_green = colors.HexColor("#15803d")       # Green 700
    c_red = colors.HexColor("#b91c1c")         # Red 700
    c_bg_card = colors.HexColor("#f8fafc")     # Slate 50
    c_border = colors.HexColor("#e2e8f0")      # Slate 200

    # Custom typography styles
    title_style = ParagraphStyle(
        'DocTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=24,
        leading=28,
        textColor=c_primary,
        spaceAfter=6
    )
    subtitle_style = ParagraphStyle(
        'DocSubTitle',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=12,
        leading=16,
        textColor=c_blue,
        spaceAfter=15
    )
    meta_style = ParagraphStyle(
        'DocMeta',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8.5,
        leading=12,
        textColor=colors.HexColor("#64748b")
    )
    h1_style = ParagraphStyle(
        'Heading1_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=14,
        leading=18,
        textColor=c_primary,
        spaceBefore=14,
        spaceAfter=8,
        keepWithNext=True
    )
    h2_style = ParagraphStyle(
        'Heading2_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=11,
        leading=14,
        textColor=c_blue,
        spaceBefore=10,
        spaceAfter=5,
        keepWithNext=True
    )
    body_style = ParagraphStyle(
        'Body_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=9,
        leading=13.5,
        textColor=c_secondary,
        spaceAfter=6
    )
    body_bold = ParagraphStyle(
        'Body_Bold_Custom',
        parent=body_style,
        fontName='Helvetica-Bold',
        textColor=c_primary
    )
    bullet_style = ParagraphStyle(
        'Bullet_Custom',
        parent=body_style,
        leftIndent=15,
        firstLineIndent=-10,
        spaceAfter=4
    )
    code_inline_style = ParagraphStyle(
        'Code_Inline',
        parent=body_style,
        fontName='Courier',
        fontSize=8.5,
        leading=11,
        textColor=c_primary
    )
    callout_style = ParagraphStyle(
        'CalloutText',
        parent=body_style,
        fontSize=8.5,
        leading=12.5,
        textColor=colors.HexColor("#1e293b")
    )

    story = []

    # ==================== COVER / TITLE SECTION ====================
    story.append(Spacer(1, 10))
    story.append(Paragraph("Memory Compaction & Garbage Collector Engine", title_style))
    story.append(Paragraph("A C++ Runtime Memory Management Engine & LISP2 Compaction System", subtitle_style))
    story.append(Paragraph("<b>Author / System:</b> Autonomous C++ Garbage Collector Engine & Runtime Simulator<br/>"
                           "<b>Date:</b> October 2026 &nbsp;|&nbsp; <b>Version:</b> 1.2 Production Release &nbsp;|&nbsp; <b>Platform:</b> macOS / POSIX", meta_style))
    story.append(Spacer(1, 10))
    story.append(HRFlowable(width="100%", thickness=1.5, color=c_blue, spaceBefore=4, spaceAfter=14))

    # ==================== EXECUTIVE SUMMARY ====================
    story.append(Paragraph("1. Executive Summary & Problem Formulation", h1_style))
    story.append(Paragraph(
        "Modern managed runtime environments—such as the <b>Java Virtual Machine (JVM)</b>, <b>V8 (Google Chrome/Node.js)</b>, "
        "and the <b>.NET CLR</b>—rely on autonomous dynamic memory management to prevent memory leaks and dangling pointers. "
        "However, simple Mark-and-Sweep allocators suffer from a severe architectural limitation: <b>External Memory Fragmentation</b>.",
        body_style
    ))
    story.append(Paragraph(
        "Over time, objects of varying sizes are allocated and deallocated at scattered locations. Although total free memory across the heap "
        "may far exceed the requirements of an incoming allocation, the free memory is fragmented into disjoint, sub-allocation-size gaps. "
        "As a result, an allocation request fails—triggering an unexpected <i>OutOfMemoryError</i>.",
        body_style
    ))
    story.append(Paragraph(
        "This project implements an end-to-end, high-performance in-memory runtime engine written in standard C++17. "
        "It couples continuous 1D heap memory tracking with a <b>Live Object Binary Search Tree</b>, a stack-based DFS Mark-and-Sweep "
        "collector, and a full <b>LISP2 Three-Pass Memory Compactor</b> that relocates surviving objects, adjusts interior reference pointers, "
        "and coalesces fragmented free holes into one continuous block.",
        body_style
    ))

    # Callout Box: Unique Feature
    callout_data = [[
        Paragraph(
            "<b>Key Highlight & Unique Architectural Feature:</b><br/>"
            "This engine explicitly demonstrates how <i>Two-Finger / LISP2 Memory Compaction</i> resolves fatal allocation deadlocks. "
            "When total free heap memory is <b>212 Bytes</b>, an incoming allocation of <b>100 Bytes</b> fails because the largest contiguous hole is "
            "only <b>40 Bytes</b>. After running compaction, all live blocks slide to low memory, consolidating a single <b>212 Byte</b> contiguous gap, "
            "allowing the 100 Byte request to succeed without increasing total heap capacity.",
            callout_style
        )
    ]]
    callout_table = Table(callout_data, colWidths=[letter[0] - 108])
    callout_table.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, -1), colors.HexColor("#eff6ff")),
        ('BOX', (0, 0), (-1, -1), 1, colors.HexColor("#bfdbfe")),
        ('PADDING', (0, 0), (-1, -1), 8),
        ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ]))
    story.append(callout_table)
    story.append(Spacer(1, 10))

    # ==================== CORE ARCHITECTURE & DATA STRUCTURES ====================
    story.append(Paragraph("2. Core Architecture & Data Structures", h1_style))
    story.append(Paragraph(
        "The engine operates entirely in user-space memory without third-party dependencies, leveraging pure C++ standard library primitives and POSIX sockets:",
        body_style
    ))

    # Architecture Table
    arch_data = [
        [Paragraph("<b>Component</b>", body_bold), Paragraph("<b>Data Structure</b>", body_bold), Paragraph("<b>Role & Algorithmic Complexity</b>", body_bold)],
        [
            Paragraph("<b>Heap Array</b>", body_style),
            Paragraph("<code>uint8_t heap[512]</code>", code_inline_style),
            Paragraph("Contiguous 1D byte array. Value <code>0</code> signifies unallocated space; non-zero values correspond to the owning Object ID.", body_style)
        ],
        [
            Paragraph("<b>Live Object Index</b>", body_style),
            Paragraph("<code>ObjectBST</code> (Self-contained BST)", code_inline_style),
            Paragraph("Maintains live objects sorted by starting memory byte offset. Guarantees <i>O(log N)</i> point lookup, in-order address sorting, and rapid gap identification.", body_style)
        ],
        [
            Paragraph("<b>Object Registry</b>", body_style),
            Paragraph("<code>std::map&lt;int, HeapObject&gt;</code>", code_inline_style),
            Paragraph("Stores metadata: unique ID, type tag (e.g., <i>Array, Buffer</i>), byte offset, size, root pointer flag, and directed reference adjacency list.", body_style)
        ],
        [
            Paragraph("<b>Reachability Graph</b>", body_style),
            Paragraph("<code>std::stack&lt;int&gt;</code> DFS Traversal", code_inline_style),
            Paragraph("Traces live reachable references beginning at Root references (simulating thread stacks/registers) to identify non-garbage nodes.", body_style)
        ],
        [
            Paragraph("<b>LISP2 Compactor</b>", body_style),
            Paragraph("Three-Pass Two-Finger Pointer Relocator", code_inline_style),
            Paragraph("Calculates destination offsets, patches graph references, and shifts data blocks to eliminate fragmentation completely.", body_style)
        ],
        [
            Paragraph("<b>POSIX HTTP Server</b>", body_style),
            Paragraph("Non-blocking BSD Sockets", code_inline_style),
            Paragraph("Lightweight embedded REST API server handling CORS preflight, serving UI assets, and JSON RPC requests.", body_style)
        ]
    ]
    arch_table = Table(arch_data, colWidths=[110, 150, 244])
    arch_table.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor("#f1f5f9")),
        ('GRID', (0, 0), (-1, -1), 0.5, c_border),
        ('TOPPADDING', (0, 0), (-1, -1), 5),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 5),
        ('VALIGN', (0, 0), (-1, -1), 'TOP'),
    ]))
    story.append(arch_table)
    story.append(Spacer(1, 12))

    # ==================== THE GARBAGE COLLECTION PIPELINE ====================
    story.append(Paragraph("3. Detailed Algorithmic Execution Pipeline", h1_style))
    story.append(Paragraph(
        "The engine operates a multi-stage memory management cycle. Each phase can run either independently or as part of a full automated cycle:",
        body_style
    ))

    # Phase 1: Mark
    story.append(Paragraph("Phase 1: Mark Phase (Stack-Based DFS Reachability)", h2_style))
    story.append(Paragraph(
        "At GC invocation, every live object in <code>objects</code> has its <code>marked</code> flag reset to <code>false</code>. "
        "The engine identifies all <b>Root Objects</b> (objects with <code>is_root == true</code>, corresponding to active local variables, globals, or registers). "
        "A depth-first traversal is executed using an explicit <code>std::stack&lt;int&gt;</code>. For every popped node, all outgoing edges in its "
        "<code>references</code> vector are inspected; unvisited targets are pushed onto the stack and marked reachable. "
        "Any object unreachable from the root set remains unmarked.",
        body_style
    ))

    # Phase 2: Sweep
    story.append(Paragraph("Phase 2: Sweep Phase (Object Reclamation & Hole Generation)", h2_style))
    story.append(Paragraph(
        "The engine scans the object registry. For every node with <code>marked == false</code>:<br/>"
        "1. Its physical byte range <code>[offset ... offset + size - 1]</code> in the heap array is wiped to <code>0</code>.<br/>"
        "2. The object node is deleted from the <code>ObjectBST</code>.<br/>"
        "3. Any references pointing to this dead node from surviving objects are purged.<br/>"
        "4. The object descriptor is erased from memory.<br/>"
        "<i>Notice:</i> Sweeping clears dead bytes in place, but does <b>not</b> shift surviving objects. This creates an alternating sequence of occupied blocks and free holes.",
        body_style
    ))

    # Phase 3: Compact
    story.append(Paragraph("Phase 3: LISP2 Compaction Algorithm (Offset & Pointer Adjustment)", h2_style))
    story.append(Paragraph(
        "To resolve fragmentation without external memory buffers, the classic <b>LISP2 Compactor</b> runs in three disciplined sequential passes:",
        body_style
    ))
    story.append(Paragraph("• <b>Pass 1 — Compute Forwarding Addresses:</b> Traverses surviving objects in ascending memory offset order (via <code>bst.inorder()</code>). Maintains an allocation watermark <code>free_ptr</code> starting at 0. Assigns <code>obj.forward_addr = free_ptr</code> and advances <code>free_ptr += obj.size</code>.", bullet_style))
    story.append(Paragraph("• <b>Pass 2 — Update Inter-Object References (Pointer Adjustment):</b> In a real JVM or V8 runtime, physical pointer references held by surviving objects are updated to point to the new destination addresses computed in Pass 1. In our engine, this pass ensures that structural relationships remain coherent across object movement.", bullet_style))
    story.append(Paragraph("• <b>Pass 3 — Relocate Live Data Blocks:</b> Copies each surviving object's memory block from its original <code>offset</code> to its <code>forward_addr</code>. Wipes the remaining high-memory space <code>[free_ptr ... HEAP_SIZE - 1]</code> to 0. Rebuilds the BST with updated offsets.", bullet_style))
    
    story.append(Spacer(1, 10))

    # ==================== STEP-BY-STEP PROOF ====================
    story.append(PageBreak())
    story.append(Paragraph("4. Step-by-Step Mathematical Proof of Compaction", h1_style))
    story.append(Paragraph(
        "Below is the exact byte-level breakdown of the checkerboard fragmentation scenario built into <code>/api/demo</code> and verified by the test suite:",
        body_style
    ))

    # Heap layout table
    step_data = [
        [Paragraph("<b>Obj ID</b>", body_bold), Paragraph("<b>Type</b>", body_bold), Paragraph("<b>Size</b>", body_bold), Paragraph("<b>Offset Range</b>", body_bold), Paragraph("<b>Status</b>", body_bold), Paragraph("<b>Post-Sweep State</b>", body_bold), Paragraph("<b>Post-Compact Offset</b>", body_bold)],
        [Paragraph("1", body_style), Paragraph("Array", body_style), Paragraph("40 B", body_style), Paragraph("0 – 39", code_inline_style), Paragraph("<font color='#15803d'><b>Live (Root)</b></font>", body_style), Paragraph("Retained", body_style), Paragraph("0 – 39 (0x000)", code_inline_style)],
        [Paragraph("2", body_style), Paragraph("Buffer", body_style), Paragraph("32 B", body_style), Paragraph("40 – 71", code_inline_style), Paragraph("<font color='#b91c1c'>Dead (Garbage)</font>", body_style), Paragraph("<b>Reclaimed (Gap 1)</b>", body_style), Paragraph("Merged", body_style)],
        [Paragraph("3", body_style), Paragraph("HashMap", body_style), Paragraph("48 B", body_style), Paragraph("72 – 119", code_inline_style), Paragraph("<font color='#15803d'><b>Live (Root)</b></font>", body_style), Paragraph("Retained", body_style), Paragraph("40 – 87 (0x028)", code_inline_style)],
        [Paragraph("4", body_style), Paragraph("String", body_style), Paragraph("36 B", body_style), Paragraph("120 – 155", code_inline_style), Paragraph("<font color='#b91c1c'>Dead (Garbage)</font>", body_style), Paragraph("<b>Reclaimed (Gap 2)</b>", body_style), Paragraph("Merged", body_style)],
        [Paragraph("5", body_style), Paragraph("Vector", body_style), Paragraph("32 B", body_style), Paragraph("156 – 187", code_inline_style), Paragraph("<font color='#15803d'><b>Live (Root)</b></font>", body_style), Paragraph("Retained", body_style), Paragraph("88 – 119 (0x058)", code_inline_style)],
        [Paragraph("6", body_style), Paragraph("Closure", body_style), Paragraph("40 B", body_style), Paragraph("188 – 227", code_inline_style), Paragraph("<font color='#b91c1c'>Dead (Garbage)</font>", body_style), Paragraph("<b>Reclaimed (Gap 3)</b>", body_style), Paragraph("Merged", body_style)],
        [Paragraph("7", body_style), Paragraph("Promise", body_style), Paragraph("44 B", body_style), Paragraph("228 – 271", code_inline_style), Paragraph("<font color='#15803d'><b>Live (Root)</b></font>", body_style), Paragraph("Retained", body_style), Paragraph("120 – 163 (0x078)", code_inline_style)],
        [Paragraph("8", body_style), Paragraph("RegExp", body_style), Paragraph("36 B", body_style), Paragraph("272 – 307", code_inline_style), Paragraph("<font color='#b91c1c'>Dead (Garbage)</font>", body_style), Paragraph("<b>Reclaimed (Gap 4)</b>", body_style), Paragraph("Merged", body_style)],
        [Paragraph("9", body_style), Paragraph("WeakMap", body_style), Paragraph("40 B", body_style), Paragraph("308 – 347", code_inline_style), Paragraph("<font color='#15803d'><b>Live (Root)</b></font>", body_style), Paragraph("Retained", body_style), Paragraph("164 – 203 (0x0a4)", code_inline_style)],
        [Paragraph("10", body_style), Paragraph("Symbol", body_style), Paragraph("32 B", body_style), Paragraph("348 – 379", code_inline_style), Paragraph("<font color='#b91c1c'>Dead (Garbage)</font>", body_style), Paragraph("<b>Reclaimed (Gap 5)</b>", body_style), Paragraph("Merged", body_style)],
        [Paragraph("11", body_style), Paragraph("Iterator", body_style), Paragraph("48 B", body_style), Paragraph("380 – 427", code_inline_style), Paragraph("<font color='#15803d'><b>Live (Root)</b></font>", body_style), Paragraph("Retained", body_style), Paragraph("204 – 251 (0x0cc)", code_inline_style)],
        [Paragraph("12", body_style), Paragraph("Proxy", body_style), Paragraph("36 B", body_style), Paragraph("428 – 463", code_inline_style), Paragraph("<font color='#b91c1c'>Dead (Garbage)</font>", body_style), Paragraph("<b>Reclaimed (Gap 6)</b>", body_style), Paragraph("Merged", body_style)],
        [Paragraph("13", body_style), Paragraph("Anchor", body_style), Paragraph("48 B", body_style), Paragraph("464 – 511", code_inline_style), Paragraph("<font color='#15803d'><b>Live (Root)</b></font>", body_style), Paragraph("Retained", body_style), Paragraph("252 – 299 (0x0fc)", code_inline_style)],
    ]
    step_table = Table(step_data, colWidths=[42, 60, 48, 75, 95, 100, 84])
    step_table.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor("#f1f5f9")),
        ('GRID', (0, 0), (-1, -1), 0.5, c_border),
        ('FONTSIZE', (0, 0), (-1, -1), 8),
        ('TOPPADDING', (0, 0), (-1, -1), 3.5),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 3.5),
        ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ]))
    story.append(step_table)
    story.append(Spacer(1, 10))

    # Metrics Box
    compaction_metrics = [
        [Paragraph("<b>Metric</b>", body_bold), Paragraph("<b>State 1: Initial Allocation</b>", body_bold), Paragraph("<b>State 2: After Sweep (Fragmented)</b>", body_bold), Paragraph("<b>State 3: After LISP2 Compaction</b>", body_bold)],
        [Paragraph("<b>Used Memory</b>", body_style), Paragraph("512 Bytes (100%)", body_style), Paragraph("300 Bytes (58.6%)", body_style), Paragraph("300 Bytes (58.6%)", body_style)],
        [Paragraph("<b>Total Free Memory</b>", body_style), Paragraph("0 Bytes (0%)", body_style), Paragraph("<b>212 Bytes</b> (41.4%)", body_style), Paragraph("<b>212 Bytes</b> (41.4%)", body_style)],
        [Paragraph("<b>Largest Free Block</b>", body_style), Paragraph("0 Bytes", body_style), Paragraph("<font color='#b91c1c'><b>40 Bytes</b></font>", body_style), Paragraph("<font color='#15803d'><b>212 Bytes</b></font> (Single block)", body_style)],
        [Paragraph("<b>Fragmentation Index</b>", body_style), Paragraph("0.0%", body_style), Paragraph("<font color='#b91c1c'><b>81.1%</b></font>", body_style), Paragraph("<font color='#15803d'><b>0.0%</b></font> (Zero fragmentation)", body_style)],
        [Paragraph("<b>Allocate 100B Result</b>", body_style), Paragraph("<font color='#b91c1c'>FAIL (No memory)</font>", body_style), Paragraph("<font color='#b91c1c'><b>FAIL (Fragmented)</b></font>", body_style), Paragraph("<font color='#15803d'><b>SUCCESS (Allocated ID 14)</b></font>", body_style)],
    ]
    metrics_table = Table(compaction_metrics, colWidths=[120, 120, 134, 130])
    metrics_table.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor("#f8fafc")),
        ('GRID', (0, 0), (-1, -1), 0.5, c_border),
        ('TOPPADDING', (0, 0), (-1, -1), 4),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
        ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ]))
    story.append(metrics_table)
    story.append(Spacer(1, 12))

    # ==================== REST API REFERENCE ====================
    story.append(Paragraph("5. Embedded HTTP REST API Specification", h1_style))
    story.append(Paragraph(
        "The engine integrates an asynchronous HTTP server listening by default on port <code>8080</code>. "
        "It supports standard CORS (<code>Access-Control-Allow-Origin: *</code>), JSON request payloads, and preflight <code>OPTIONS</code> requests:",
        body_style
    ))

    api_specs = [
        [Paragraph("<b>Method & Route</b>", body_bold), Paragraph("<b>Request Body</b>", body_bold), Paragraph("<b>Response Payload</b>", body_bold), Paragraph("<b>Description</b>", body_bold)],
        [
            Paragraph("<code>GET /</code><br/><code>GET /index.html</code>", code_inline_style),
            Paragraph("<i>None</i>", body_style),
            Paragraph("HTML Web Visualizer", body_style),
            Paragraph("Renders live UI with 64x8 heap grid, BST tree canvas, graph canvas, and event log.", body_style)
        ],
        [
            Paragraph("<code>GET /api/state</code>", code_inline_style),
            Paragraph("<i>None</i>", body_style),
            Paragraph("Full JSON engine state", body_style),
            Paragraph("Returns 512-byte array, object metadata, reference lists, BST nodes, stats, and last 100 log lines.", body_style)
        ],
        [
            Paragraph("<code>POST /api/allocate</code>", code_inline_style),
            Paragraph("<code>{\"name\":\"Buffer\", \"size\":64, \"is_root\":true, \"with_gc\":false}</code>", code_inline_style),
            Paragraph("<code>{\"success\":true, \"id\":14}</code> or failure with gap stats", body_style),
            Paragraph("Allocates byte range. If <code>with_gc</code> is true, automatically triggers GC cycle upon failure.", body_style)
        ],
        [
            Paragraph("<code>POST /api/reference</code>", code_inline_style),
            Paragraph("<code>{\"from\":1, \"to\":5}</code>", code_inline_style),
            Paragraph("<code>{\"success\":true}</code>", body_style),
            Paragraph("Adds a directed reference link between two active objects for reachability analysis.", body_style)
        ],
        [
            Paragraph("<code>POST /api/remove-root</code>", code_inline_style),
            Paragraph("<code>{\"id\":2}</code>", code_inline_style),
            Paragraph("<code>{\"success\":true}</code>", body_style),
            Paragraph("Clears the root bit, converting node into collectible garbage if unreferenced.", body_style)
        ],
        [
            Paragraph("<code>POST /api/gc</code>", code_inline_style),
            Paragraph("<code>{\"phase\":\"full\"}</code> | <code>\"mark\"</code> | <code>\"sweep\"</code> | <code>\"compact\"</code>", code_inline_style),
            Paragraph("Updated state JSON", body_style),
            Paragraph("Triggers complete Mark-Sweep-Compact or executes an individual phase for inspection.", body_style)
        ],
        [
            Paragraph("<code>POST /api/demo</code>", code_inline_style),
            Paragraph("<i>None</i>", body_style),
            Paragraph("Updated state JSON", body_style),
            Paragraph("Sets up the verified 13-object checkerboard fragmentation dilemma.", body_style)
        ],
        [
            Paragraph("<code>POST /api/reset</code>", code_inline_style),
            Paragraph("<i>None</i>", body_style),
            Paragraph("Updated state JSON", body_style),
            Paragraph("Flushes all objects, wipes heap array to zero, and resets internal counters.", body_style)
        ],
    ]
    api_table = Table(api_specs, colWidths=[110, 135, 120, 139])
    api_table.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor("#f1f5f9")),
        ('GRID', (0, 0), (-1, -1), 0.5, c_border),
        ('FONTSIZE', (0, 0), (-1, -1), 8),
        ('TOPPADDING', (0, 0), (-1, -1), 4),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
        ('VALIGN', (0, 0), (-1, -1), 'TOP'),
    ]))
    story.append(api_table)
    story.append(Spacer(1, 12))

    # ==================== VERIFICATION & TESTING ====================
    story.append(Paragraph("6. Verification & Automated Test Commands", h1_style))
    story.append(Paragraph(
        "To verify this runtime engine on any Unix/POSIX system, execute the provided automated walkthrough script or cURL tests:",
        body_style
    ))

    code_snippet = (
        "# 1. Launch backend daemon on port 8080\n"
        "./gc_engine 8080\n\n"
        "# 2. Run the automated colored terminal demonstration\n"
        "python3 demo_walkthrough.py\n\n"
        "# 3. Open the rich visualization dashboard in any browser\n"
        "open http://localhost:8080\n"
    )
    story.append(Preformatted(code_snippet, ParagraphStyle('CodeBlock', parent=styles['Normal'], fontName='Courier', fontSize=8, leading=11, textColor=colors.HexColor("#1e293b"))))

    story.append(Spacer(1, 10))
    story.append(Paragraph(
        "<b>Summary Conclusion:</b> The Memory Compaction & Garbage Collector Engine provides a comprehensive, rigorous simulation "
        "of production-grade JVM and V8 heap management. By marrying an explicit Binary Search Tree live-object index with the "
        "three-pass LISP2 Compaction algorithm, it completely resolves fragmentation bottlenecks and ensures maximum memory utilization.",
        body_style
    ))

    # Build PDF
    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"✓ PDF generated successfully: {PDF_OUTPUT_PATH}")

if __name__ == "__main__":
    build_pdf()
