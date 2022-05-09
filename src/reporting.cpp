#include "reporting.h"
#include "utils/utf8.h"

void report(ThreadData *ctx, FILE *out, const char *levelPrefix, int fileIndex, uint32_t offset0, uint32_t offset1, Str message) {
  auto fileEntry = file(ctx->globalData, fileIndex);

  int line = 1;
  int col = 1;

  auto begin = fileEntry.content.data;
  auto it = begin;
  auto lastLineStart = begin;
  auto end = fileEntry.content.data + fileEntry.content.len;

  int line0 = 1;
  int col0 = 1;
  int line1 = 1;
  int col1 = 1;
  const char *line0Start = begin;
  const char *line1Start = begin;

  while (it < end) {
    if ((it - begin) == offset0) {
      line0Start = lastLineStart;
      line0 = line;
      col0 = col;
    }
    if ((it - begin) == offset1) {
      line1Start = lastLineStart;
      line1 = line;
      col1 = col;
      break;
    }

    int ch = utf8advance(&it, end);
    if (ch == '\n') {
      line += 1;
      col = 1;
      lastLineStart = it;
    } else {
      col += 1;
    }
  }

  const char *line0End = line0Start;
  while (*line0End != '\n') ++line0End;

  auto prefixLen = snprintf(NULL, 0, "%.*s:%d:%d",
    (int)fileEntry.relativePath.len,
    fileEntry.relativePath.data,
    line0, col0);

  fprintf(out, "%.*s:%d:%d| %s %.*s\n",
    (int)fileEntry.relativePath.len,
    fileEntry.relativePath.data,
    line0, col0, 
    levelPrefix,
    (int) message.len, message.data
    );

  fprintf(out, "%.*s:%d:%d",
    (int)fileEntry.relativePath.len,
    fileEntry.relativePath.data,
    line0, col0);

  fprintf(out, "|%.*s\n",
    (int)(line0End - line0Start),
    line0Start
    );

  for (int i = 0; i < prefixLen; ++i) fprintf(out, " ");
  fprintf(out, "|");
  for (int i = 0; i < col0 - 1; ++i) fprintf(out, " ");
  fprintf(out, "^");
  fprintf(out, "\n");


}
