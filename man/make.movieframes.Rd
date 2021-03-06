% Generated by roxygen2 (4.0.0): do not edit by hand
\name{make.movieframes}
\alias{make.movieframes}
\title{Generate a movie of MEA activity.}
\usage{
make.movieframes(x, beg = 1, end = dim(x$rates$rates)[1],
  outputdir = dirname(tempfile()), prefix = "mea",
  show.frames = interactive(), seconds = TRUE, delete.first = TRUE,
  clean.after = FALSE, anim.delay = 5)
}
\arguments{
\item{x}{The "s" object.}

\item{beg}{start time of the movie}

\item{end}{end time of the movie}

\item{outputdir}{directory to store the frames (no slash at end).
If directory does not exist, it is created first.}

\item{prefix}{prefix file name for frames}

\item{show.frames}{Boolean -- do we show the frames on screen as well?}

\item{seconds}{Boolean: draw the time above the plot?}

\item{delete.first}{Boolean: delete the outputdir before starting?}

\item{clean.after}{Boolean: delete the outputdir after finishing?}

\item{anim.delay}{time (in seconds) delay between frames.  If delay is zero,
do not convert movie.}
}
\value{
NULL.
}
\description{
The mean firing rate of each unit is computed and represented as a circle with
the area proportional to the firing rate.  The sequence of frames are then
coerced into a movie.
}
\author{
Stephen Eglen
}

